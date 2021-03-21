// C++
#include <vector>
#include <iostream>

// xwmfs
#include "fuse/SymlinkEntry.hxx"
#include "main/DesktopsRootDir.hxx"
#include "main/DesktopDirEntry.hxx"
#include "x11/RootWin.hxx"

namespace xwmfs
{

DesktopsRootDir::DesktopsRootDir(RootWin &root) :
	DirEntry("desktops"),
	m_root_win(root)
{
}

typedef std::map<size_t, std::vector<const XWindow*>> WindowMap;

static WindowMap buildWindowMap(const RootWin &root_win)
{
	WindowMap ret;

	for( const auto &window: root_win.getWindowList() )
	{
		try
		{
			auto desktop_nr = window.getCachedDesktop();
			ret[desktop_nr].push_back(&window);
		}
		catch( const xwmfs::Exception &ex )
		{
			// has no desktop assignment for some reason
			continue;
		}
	}

	return ret;
}

void DesktopsRootDir::handleDesktopsChanged()
{
	// Currently we rebuild the complete structure every time desktop
	// names change or desktops (dis)appear. This could be more
	// efficient by only applying incremental changes but also more
	// complex to do right.
	this->clear();
	m_window_desktop_dir_map.clear();

	m_root_win.queryWindows();

	const auto window_map = buildWindowMap(m_root_win);

	const auto &desktops = m_root_win.getDesktopNames();

	for( size_t i = 0; i < desktops.size(); i++ )
	{
		auto desktop_dir = new DesktopDirEntry(i, desktops[i]);
		this->addEntry(desktop_dir);

		auto window_it = window_map.find(i);
		if( window_it == window_map.end() )
			continue;
		auto &windows = window_it->second;

		for( auto window: windows )
		{
			addWindowToDesktop( desktop_dir, window );
		}
	}
}

void DesktopsRootDir::addWindowToDesktop(DesktopDirEntry *dir, const XWindow *window)
{
	const auto &winid = window->idStr();
	const auto target = std::string("../../../windows/") + winid;

	auto symlink = new SymlinkEntry(winid, target);
	dir->getWindowsDir()->addEntry(symlink);

	m_window_desktop_dir_map.insert( {window->id(), dir} );
}

void DesktopsRootDir::removeWindow(const XWindow *window)
{
	auto it = m_window_desktop_dir_map.find(window->id());

	if( it == m_window_desktop_dir_map.end() )
		// unknown for some reason
		return;

	auto windows_dir = it->second->getWindowsDir();
	windows_dir->removeEntry(window->idStr());
	m_window_desktop_dir_map.erase(it);
}

DesktopDirEntry* DesktopsRootDir::getDesktopDir(const size_t desktop_nr)
{
	auto desktop_str = std::to_string(desktop_nr);
	auto ret = this->getDirEntry(desktop_str);
	return reinterpret_cast<DesktopDirEntry*>(ret);
}

void DesktopsRootDir::handleWindowCreated(const XWindow &w)
{
	int desktop_nr = -1;

	try
	{
		desktop_nr = w.getDesktop();
	}
	catch( const xwmfs::XWindow::PropertyNotExisting & )
	{
		// no desktop assigned yet
		return;
	}

	auto desktop_entry = this->getDesktopDir(desktop_nr);

	if( !desktop_entry )
		// could be a race condition, new window created but also
		// the desktop structure changed in the meantime. Should be
		// covered by handleDesktopsChanged().
		return;

	addWindowToDesktop( desktop_entry, &w );
}

void DesktopsRootDir::handleWindowDestroyed(const XWindow &w)
{
	removeWindow(&w);
}

void DesktopsRootDir::handleWindowDesktopChanged(const XWindow &w)
{
	auto it = m_window_desktop_dir_map.find(w.id());

	if( it == m_window_desktop_dir_map.end() )
	{
		// so maybe this is the first time a value is assigned for the
		// desktop, do a plain add
		return handleWindowCreated(w);
	}

	const auto desktop_nr = w.getDesktop();
	auto desktop_dir = it->second;

	if( desktop_nr < 0 )
		// should not happen
		return;

	if( static_cast<size_t>(desktop_nr) == desktop_dir->getDesktopNr() )
		// unchanged for some reason
		return;

	removeWindow(&w);
	addWindowToDesktop(desktop_dir, &w);
}

} // end ns
