#pragma once

// libxpp
#include <xpp/fwd.hxx>

// xwmfs
#include "fuse/FileEntry.hxx"

namespace xwmfs {

class WinManagerWindow;

/// A FileEntry that is associated with global window manager data.
/**
 * This is a specialized FileEntry for particular global entries relating to
 * the window manager. Mostly this is only used for writable files to relay
 * the write request correctly.
 **/
struct WinManagerFileEntry :
		public FileEntry {

	WinManagerFileEntry(const std::string &n, const cosmos::RealTime &t = cosmos::RealTime{}) :
			FileEntry{n, t, Writable{true}} {
	}

	Bytes write(OpenContext *ctx, const char *data,
			const size_t bytes, off_t offset) override;
protected: // functions

	void callUpdateFunc(const int value) const;
};

} // end ns
