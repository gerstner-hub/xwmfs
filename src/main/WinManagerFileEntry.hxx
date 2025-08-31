#pragma once

// C++
#include <map>

// xwmfs
#include "fuse/FileEntry.hxx"

namespace xpp {
	class XWindow;
}

namespace xwmfs {

class WinManagerWindow;

/**
 * \brief
 * 	A FileEntry that is associated with a global window manager entry
 * \details
 * 	This is a specialized FileEntry for particular global entries relating
 * 	to the window manager. Mostly this is only used for writable files to
 * 	relay the write request correctly.
 **/
struct WinManagerFileEntry :
		public FileEntry {

	WinManagerFileEntry(const std::string &n, const time_t t = 0) :
			FileEntry{n, Writable{true}, t} {
	}

	int write(OpenContext *ctx, const char *data, const size_t bytes, off_t offset) override;

protected: // types

	using SetIntFunction = void (WinManagerWindow::*)(const int);
	using SetIntFunctionMap = std::map<std::string, SetIntFunction>;
	using SetWindowFunction = void (WinManagerWindow::*)(const xpp::XWindow&);
	using SetWindowFunctionMap = std::map<std::string, SetWindowFunction>;

	// a mapping of file system names to their associated set int functions
	static const SetIntFunctionMap m_set_int_function_map;
	static const SetWindowFunctionMap m_set_window_function_map;
};

} // end ns
