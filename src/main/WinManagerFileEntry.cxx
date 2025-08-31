// C++
#include <map>

// libxpp
#include <xpp/XWindow.hxx>

// xwmfs
#include "main/WinManagerFileEntry.hxx"
#include "main/Xwmfs.hxx"
#include "main/logger.hxx"
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

int WinManagerFileEntry::write(OpenContext *ctx, const char *data,
		const size_t bytes, off_t offset) {
	(void)ctx;
	if (!m_writable)
		return -EBADF;
	// we don't support writing at offsets
	if (offset)
		return -EOPNOTSUPP;

	try {
		int the_num = 0;

		if (const auto parsed = parseInteger(data, bytes, the_num);
				parsed < 0) {
			logger->warn() << __FUNCTION__
				<<": Failed to parse integer for write to: "
				<< this->m_name << "\n";
			return parsed;
		}

		cosmos::MutexGuard g{m_parent->getLock()};

		if (auto err = callUpdateFunc(the_num); err != 0) {
			return err;
		}
	} catch (const xpp::XWindow::NotImplemented &e) {
		return -ENOSYS;
	} catch (const std::exception &e) {
		logger->error()
			<< __FUNCTION__ << ": Error setting window manager property ("
			<< this->m_name << "): " << e.what() << std::endl;
		return -EINVAL;
	}

	/*
	 * Claim we've written all content. This "eats up" things like
	 * newlines passed with 'echo'. Otherwise programs may try to write
	 * the "missing" bytes which turns into an offset write then, which we
	 * don't support
	 */
	return bytes;
}

int WinManagerFileEntry::callUpdateFunc(const int value) const {
	auto &root_win = xwmfs::Xwmfs::getInstance().getRootWin();

	if (auto int_it = SET_INT_FUNCTION_MAP.find(m_name);
			int_it != SET_INT_FUNCTION_MAP.end()) {
		auto set_func = int_it->second;
		((root_win).*set_func)(value);
		return 0;
	}

	if (auto window_it = SET_WINDOW_FUNCTION_MAP.find(m_name);
			window_it != SET_WINDOW_FUNCTION_MAP.end()) {
		if (value < 0) {
			// there are no negative window numbers
			return -EINVAL;
		}
		auto set_func = window_it->second;
		((root_win).*set_func)(xpp::XWindow(xpp::WinID{static_cast<Window>(value)}));
		return 0;
	}

	logger->warn()
		<< __FUNCTION__
		<< ": Write call for win manager file of unknown type: \""
		<< this->m_name
		<< "\"\n";
	return -ENXIO;
}

} // end ns
