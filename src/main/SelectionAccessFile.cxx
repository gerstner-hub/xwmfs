// libxpp
#include <xpp/AtomMapper.hxx>

// xwmfs
#include "fuse/AbortHandler.hxx"
#include "fuse/xwmfs_fuse.hxx"
#include "main/Exception.hxx"
#include "main/logger.hxx"
#include "main/SelectionAccessFile.hxx"
#include "main/SelectionDirEntry.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs {

SelectionAccessFile::SelectionAccessFile(const std::string &n,
			SelectionDirEntry &parent, const xpp::AtomID type) :
		FileEntry{n, Writable{true}},
		m_parent{parent},
		m_sel_type{type},
		m_target_prop{xpp::atom_mapper.mapAtom(n)},
		m_result_cond{parent.getLock()} {
	this->createAbortHandler(m_result_cond);
}

int SelectionAccessFile::read(
		OpenContext *ctx, char *buf, size_t size, off_t offset) {
	updateOwner();
	auto &xwmfs = xwmfs::Xwmfs::getInstance();

	if (!m_owner.valid()) {
		// no one owns the selection at the moment
		return -EAGAIN;
	} else if (m_owner != xwmfs.getSelectionWindow()) {
		// stupid situation here, see EventFile::read
		FileSysRevReadGuard guard(xwmfs.getFS());
		const auto res = updateSelection();

		if (res != 0) {
			return res;
		}
	}
	// else: we ourselves own the selection, so just return our local node
	// data

	return FileEntry::read(ctx, buf, size, offset);
}

int SelectionAccessFile::write(
		OpenContext *ctx, const char *data, const size_t bytes, off_t offset) {
	(void)ctx;

	// we don't support writing at offsets
	if (offset) {
		return -EOPNOTSUPP;
	}

	auto &sel_window = xwmfs::Xwmfs::getInstance().getSelectionWindow();

	sel_window.makeSelectionOwner(m_sel_type, CurrentTime);

	// store the data for later requests to provide the selection buffer
	this->str("");
	(*this) << std::string(data, bytes);

	return bytes;
}

void SelectionAccessFile::reportConversionResult(const xpp::AtomID result_prop) {
	{
		cosmos::MutexGuard g{m_parent.getLock()};
		m_result_arrived = true;
		m_result_prop = result_prop;
	}

	m_result_cond.signal();
}

void SelectionAccessFile::provideConversion(xpp::XWindow &requestor,
		const xpp::AtomID target_prop) const {
	cosmos::MutexGuard g{m_parent.getLock()};
	const auto copy = this->str();
	xpp::Property<xpp::utf8_string> data{xpp::utf8_string{copy.c_str()}};
	requestor.setProperty(target_prop, data);
}

int SelectionAccessFile::updateSelection() {
	auto &xwmfs = Xwmfs::Xwmfs::getInstance();
	auto &sel_win = xwmfs.getSelectionWindow();

	cosmos::MutexGuard g{m_parent.getLock()};

	m_result_prop = xpp::AtomID::INVALID;
	m_result_arrived = false;

	sel_win.convertSelection(
		m_sel_type,
		// currently we hard-codedly deal with utf8 data
		// -> this could be expanded later on by passing the desired
		// mime type some way
		xpp::atoms::ewmh_utf8_string,
		m_target_prop
	);

	// wait until the event thread reports the conversion data has arrived
	while (!m_result_arrived) {
		if (m_abort_handler->wasAborted()) {
			return -EINTR;
		} else if (!m_abort_handler->prepareBlockingCall(this)) {
			return -EINTR;
		}

		m_result_cond.wait();
		m_abort_handler->finishedBlockingCall();
	}

	if (m_result_prop == xpp::AtomID::INVALID) {
		// conversion was not possible
		xwmfs::logger->error()
			<< "Selection conversion for "
			<< cosmos::to_integral(m_sel_type) << " failed.";
		return -EIO;
	} else if (m_result_prop != m_target_prop) {
		// was written to a different property?!
		xwmfs::logger->error()
			<< "Selection conversion was sent to property "
			<< cosmos::to_integral(m_result_prop) << " instead of "
			<< cosmos::to_integral(m_target_prop);
		return -EIO;
	}

	try {
		xpp::Property<xpp::utf8_string> selection_data;
		sel_win.getProperty(m_target_prop, selection_data);

		this->str("");
		(*this) << selection_data.get().str;
	} catch (const std::exception &ex) {
		xwmfs::logger->error()
			<< "Failed to acquire selection buffer conversion data: "
			<< ex.what();
		return -EIO;
	}

	return 0;
}

void SelectionAccessFile::updateOwner() {
	// needs the event lock to avoid issues in libX11 with multi-threading
	auto &xwmfs = Xwmfs::Xwmfs::getInstance();
	cosmos::MutexGuard g{xwmfs.getEventLock()};

	m_owner = xpp::XWindow{m_parent.getSelectionOwner(m_sel_type)};
}

} // end ns
