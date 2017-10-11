// xwmfs
#include "fuse/AbortHandler.hxx"
#include "fuse/xwmfs_fuse.hxx"
#include "main/SelectionAccessFile.hxx"
#include "main/SelectionDirEntry.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs
{

SelectionAccessFile::SelectionAccessFile(
	const std::string &n,
	SelectionDirEntry &parent,
	const XAtom &type
) :
	FileEntry(n, true, 0),
	m_parent(parent),
	m_sel_type(type),
	m_target_prop(XAtomMapper::getInstance().getAtom("CONVERSION")),
	m_result_cond(parent.getLock())
{
	this->createAbortHandler(m_result_cond);
}

int SelectionAccessFile::read(
	OpenContext *ctx, char *buf, size_t size, off_t offset
)
{
	updateOwner();

	if( !m_owner.valid() )
	{
		// no one owns the selection at the moment
		return -EAGAIN;
	}

	{
		// stupid situation here, see EventFile::read
		FileSysRevReadGuard guard( xwmfs::Xwmfs::getInstance().getFS() );
		const auto res = updateSelection();

		if( res != 0 )
		{
			return res;
		}
	}

	return FileEntry::read(ctx, buf, size, offset);
}

void SelectionAccessFile::reportConversionResult(Atom result_prop)
{
	{
		MutexGuard g(m_parent.getLock());
		m_result_arrived = true;
		m_result_prop = result_prop;
	}

	m_result_cond.signal();
}

int SelectionAccessFile::updateSelection()
{
	auto &xwmfs = Xwmfs::Xwmfs::getInstance();
	auto &sel_win = xwmfs.getSelectionWindow();
	auto &std_props = StandardProps::instance();

	MutexGuard g(m_parent.getLock());

	m_result_prop.reset();
	m_result_arrived = false;

	sel_win.convertSelection(
		m_sel_type,
		// currently we hard-codedly deal with utf8 data
		// -> this could be expanded later on by passing the desired
		// mime type some way
		std_props.atom_ewmh_utf8_string,
		m_target_prop
	);

	// wait until the event thread reports the conversion data has arrived
	while( ! m_result_arrived )
	{
		if( m_abort_handler->wasAborted() )
		{
			return -EINTR;
		}

		if( ! m_abort_handler->prepareBlockingCall(this) )
		{
			return -EINTR;
		}
		m_result_cond.wait();
		m_abort_handler->finishedBlockingCall();
	}

	if( ! m_result_prop.valid() )
	{
		// conversion was not possible
		xwmfs::StdLogger::getInstance().error()
			<< "Selection conversion for "
			<< m_sel_type << " failed.";
		return -EIO;
	}
	else if( m_result_prop != m_target_prop )
	{
		// was written to a different property?!
		xwmfs::StdLogger::getInstance().error()
			<< "Selection conversion was sent to "
			<< m_result_prop << " instead of " <<
			m_target_prop;
		return -EIO;
	}

	try
	{
		Property<utf8_string> selection_data;
		sel_win.getProperty(m_target_prop, selection_data);

		this->str("");
		(*this) << selection_data.get().data;
	}
	catch( const xwmfs::Exception &ex )
	{
		xwmfs::StdLogger::getInstance().error()
			<< "Failed to acquire selection buffer conversion data: "
			<< ex;
		return -EIO;
	}

	return 0;
}

void SelectionAccessFile::updateOwner()
{
	// needs the event lock to avoid issues in libX11 with multi-threading
	auto &xwmfs = Xwmfs::Xwmfs::getInstance();
	MutexGuard g(xwmfs.getEventLock());

	m_owner = XWindow(m_parent.getSelectionOwner(m_sel_type));
}

} // end ns

