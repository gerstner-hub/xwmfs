#pragma once

// C++
#include <map>

// xpp
#include <xpp/fwd.hxx>
#include <xpp/XWindow.hxx>

// xwmfs
#include "fuse/DirEntry.hxx"

namespace xwmfs {

class DesktopDirEntry;
class WinManagerWindow;

/**
 * \brief
 * 	A directory containing per-desktop window information
 * \details
 * 	This directory keeps a sub-directory for each known desktop. Desktops
 * 	don't have unique IDs but only have an index that can change over
 * 	time. So the first desktop has index 0 and so on.
 *
 * 	Each desktop sub-directory contains a `name` node containing the
 * 	actual desktop name and a `windows` sub-directory which contains
 * 	symlinks to each window present on the respective desktop. The
 * 	symlinks point towards the top-level `windows/<id>` directory where
 * 	detailed window information can be obtained.
 **/
class DesktopsRootDir :
	public DirEntry {
public: // functions

	DesktopsRootDir(WinManagerWindow &root);

	void handleDesktopsChanged();

	void handleWindowDesktopChanged(const xpp::XWindow &w);

	void handleWindowDestroyed(const xpp::XWindow &w);

	void handleWindowCreated(const xpp::XWindow &w);

protected: // functions

	void addWindowToDesktop(DesktopDirEntry *dir, const xpp::XWindow &window);
	void removeWindow(const xpp::XWindow &window);

	DesktopDirEntry* getDesktopDir(const size_t desktop_nr);

protected: // data

	WinManagerWindow &m_root_win;
	std::map<xpp::WinID, DesktopDirEntry*> m_window_desktop_dir_map;
};

} // end ns
