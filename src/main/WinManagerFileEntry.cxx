// C++
#include <map>

// libxpp
#include <xpp/XWindow.hxx>

// xwmfs
#include "main/logger.hxx"
#include "main/WinManagerFileEntry.hxx"
#include "main/Xwmfs.hxx"
#include "x11/WinManagerWindow.hxx"

namespace xwmfs {

using SetIntFunction = void (WinManagerWindow::*)(const int);
using SetIntFunctionMap = std::map<std::string, SetIntFunction>;
using SetWindowFunction = void (WinManagerWindow::*)(const xpp::XWindow&);
using SetWindowFunctionMap = std::map<std::string, SetWindowFunction>;

// mappings of file system names to their associated setter functions
const SetIntFunctionMap SET_INT_FUNCTION_MAP = {
	{"active_desktop", &WinManagerWindow::setActiveDesktop},
	{"number_of_desktops", &WinManagerWindow::setNumDesktops}
};

const SetWindowFunctionMap SET_WINDOW_FUNCTION_MAP = {
	{"active_window", &WinManagerWindow::setActiveWindow}
};

Entry::Bytes WinManagerFileEntry::write(OpenContext *ctx, const char *data,
		const size_t bytes, off_t offset) {
	(void)ctx;
	if (!m_writable) {
		throw cosmos::Errno::BAD_FD;
	} else if (offset) {
		// we don't support writing at offsets
		throw cosmos::Errno::OP_NOT_SUPPORTED;
	}

	try {
		int the_num = 0;

		try {
			parseInteger(data, bytes, the_num);
		} catch (const cosmos::Errno err) {
			logger->warn() << __FUNCTION__
				<<": Failed to parse integer for write to: "
				<< this->m_name << ": " << err << "\n";
			throw;
		}

		cosmos::MutexGuard g{m_parent->getLock()};

		callUpdateFunc(the_num);
	} catch (const xpp::XWindow::NotImplemented &e) {
		throw cosmos::Errno::NO_SYS;
	} catch (const std::exception &e) {
		logger->error()
			<< __FUNCTION__ << ": Error setting window manager property ("
			<< this->m_name << "): " << e.what() << std::endl;
		throw cosmos::Errno::INVALID_ARG;
	}

	/*
	 * Claim we've written all content. This "eats up" things like
	 * newlines passed with 'echo'. Otherwise programs may try to write
	 * the "missing" bytes which turns into an offset write then, which we
	 * don't support
	 */
	return Bytes{static_cast<int>(bytes)};
}

void WinManagerFileEntry::callUpdateFunc(const int value) const {
	auto &root_win = xwmfs::Xwmfs::getInstance().getRootWin();

	if (auto int_it = SET_INT_FUNCTION_MAP.find(m_name);
			int_it != SET_INT_FUNCTION_MAP.end()) {
		auto set_func = int_it->second;
		((root_win).*set_func)(value);
		return;
	}

	if (auto window_it = SET_WINDOW_FUNCTION_MAP.find(m_name);
			window_it != SET_WINDOW_FUNCTION_MAP.end()) {
		if (value < 0) {
			// there are no negative window numbers
			throw cosmos::Errno::INVALID_ARG;
		}
		auto set_func = window_it->second;
		((root_win).*set_func)(xpp::XWindow(xpp::WinID{static_cast<Window>(value)}));
		return;
	}

	logger->warn()
		<< __FUNCTION__
		<< ": Write call for win manager file of unknown type: \""
		<< this->m_name
		<< "\"\n";
	throw cosmos::Errno::NXIO;
}

} // end ns
