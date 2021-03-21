#ifndef XWMFS_DESKTOPS_ROOT_DIR_HXX
#define XWMFS_DESKTOPS_ROOT_DIR_HXX

// C++
#include <map>

// xwmfs
#include "fuse/DirEntry.hxx"
#include "x11/XWindow.hxx"

namespace xwmfs
{

class DesktopDirEntry;
class RootWin;

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
	public DirEntry
{
public: // functions

	DesktopsRootDir(RootWin &root);

	void handleDesktopsChanged();

	void handleWindowDesktopChanged(const XWindow &w);

	void handleWindowDestroyed(const XWindow &w);

	void handleWindowCreated(const XWindow &w);

protected: // functions

	void addWindowToDesktop(DesktopDirEntry *dir, const XWindow *window);
	void removeWindow(const XWindow *window);

	DesktopDirEntry* getDesktopDir(const size_t desktop_nr);

protected: // data

	RootWin &m_root_win;
	std::map<Window, DesktopDirEntry*> m_window_desktop_dir_map;
};

} // end ns

#endif // inc. guards
