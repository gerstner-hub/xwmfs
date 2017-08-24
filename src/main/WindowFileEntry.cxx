// xwmfs
#include "main/WindowFileEntry.hxx"
#include "main/StdLogger.hxx"
#include "x11/XWindowAttrs.hxx"
#include "common/Helper.hxx"
#include "fuse/DirEntry.hxx"

namespace xwmfs
{

const WindowFileEntry::WriteMemberFunctionMap
	WindowFileEntry::m_write_member_function_map =
{
	{ "name", &WindowFileEntry::writeName },
	{ "desktop", &WindowFileEntry::writeDesktop },
	{ "control", &WindowFileEntry::writeCommand },
	{ "geometry", &WindowFileEntry::writeGeometry }
};

void WindowFileEntry::writeGeometry(const char *data, const size_t bytes)
{
	std::stringstream ss;
	ss.str(std::string(data, bytes));

	char c = '\0';
	XWindowAttrs attrs;
	bool good = true;

	ss >> attrs.x;
	good = good && ss.good();
	ss >> c;
	good = good && c == ',';
	ss >> attrs.y;
	good = good && ss.good();
	ss >> c;
	good = good && c == ':';
	ss >> attrs.width;
	good = good && ss.good();
	ss >> c;
	good = good && c == 'x';
	ss >> attrs.height;
	good = good && ss.good();

	if( !good )
	{
		xwmfs_throw(
			xwmfs::Exception("Couldn't parse new geometry")
		);
	}

	m_win.moveResize(attrs);
	// send the move request out immediately
	XDisplay::getInstance().flush();
}

void WindowFileEntry::writeCommand(const char *data, const size_t bytes)
{
	const auto command = tolower(stripped(std::string(data, bytes)));

	if( command == "destroy" )
	{
		m_win.destroy();
	}
	else if( command == "delete" )
	{
		m_win.sendDeleteRequest();
	}
	else
	{
		xwmfs_throw(
			xwmfs::Exception("invalid command encountered")
		);
	}
}

int WindowFileEntry::write(OpenContext *ctx, const char *data, const size_t bytes, off_t offset)
{
	(void)ctx;
	if( ! m_writable )
		return -EBADF;
	// we don't support writing at offsets
	if( offset )
		return -EOPNOTSUPP;

	try
	{
		auto it = m_write_member_function_map.find(m_name);

		if( it == m_write_member_function_map.end() )
		{
			xwmfs::StdLogger::getInstance().error()
				<< __FUNCTION__
				<< ": Write call for window file entry of unknown type: \""
				<< this->m_name
				<< "\"\n";
			return -ENXIO;
		}

		auto mem_fn = it->second;

		MutexGuard g(m_parent->getLock());

		(this->*(mem_fn))(data, bytes);
	}
	catch( const xwmfs::Exception &e )
	{
		xwmfs::StdLogger::getInstance().error()
			<< __FUNCTION__
			<< ": Error operating on window (node '"
			<< this->m_name << "'): "
			<< e.what() << std::endl;
		return -EINVAL;
	}

	return bytes;
}

} // end ns

