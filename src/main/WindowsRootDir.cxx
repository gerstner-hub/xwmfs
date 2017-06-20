// C++
#include <sstream>

// xwmfs
#include "x11/XWindow.hxx"
#include "main/WindowsRootDir.hxx"
#include "main/WindowDirEntry.hxx"
#include "main/StdLogger.hxx"

namespace xwmfs
{

WindowsRootDir::WindowsRootDir() :
	DirEntry("windows")
{

}

/*
 *	Upon a destroy event for a window this function is called for the
 *	according window.
 *
 *	It is possible that the given window isn't existing in the filesystem
 *	in which case the event should be ignored.
 *
 *	Exceptions are handled by the caller
 */
void WindowsRootDir::removeWindow(const XWindow &win)
{
	std::stringstream id;
	id << win.id();

	removeEntry(id.str());
}

WindowDirEntry* WindowsRootDir::getWindowDir(const XWindow &win)
{
	std::stringstream id;
	id << win.id();

	// TODO: this is an unsafe cast, because we have no type information
	// for WindowDirEntry ... :-/
	auto win_dir = reinterpret_cast<WindowDirEntry*>(
		getDirEntry(id.str().c_str())
	);

	return win_dir;
}

/*
 *	Upon create event for a window this function is called for the
 *	according window
 */
void WindowsRootDir::addWindow(
	const XWindow &win, const bool initial, const bool is_root_win
)
{
	if( ! is_root_win )
	{
		// we want to get any structure change events
		//
		// but don't register these for the root window, Xwmfs class
		// already registered events for that one. Otherwise we'd
		// overwrite settings like getting create events.
		win.selectDestroyEvent();
		win.selectPropertyNotifyEvent();
	}

	// make sure the XServer knows we want to get those events, otherwise
	// race conditions can occur so that for example:
	// - we see that the new window has no "name" yet
	// - the XServer didn't get our event registration yet, sets a name
	// for the window but doesn't notify us
	// - so in the end we'd never get to know about the window name
	XDisplay::getInstance().sync();

	auto win_dir = new xwmfs::WindowDirEntry(win, initial ? true : false);

	auto &logger = xwmfs::StdLogger::getInstance();

	try
	{
		// the window directories are named after their IDs
		addEntry(win_dir, false);
		logger.debug() << "Added window " << win.id() << std::endl;
	}
	catch( const DirEntry::DoubleAddError & )
	{
		/*
		 * this situation happens sometimes e.g. on i3 window manager.
		 * a window is destroyed but some kind of zombie entry remains
		 * in the client list. if xwmfs starts up in this situation
		 * then it will populate this zombie window in the file
		 * system, however all operations on it will fail, thus many
		 * directory nodes will be missing.
		 *
		 * when a new window is created then i3 seems to recycle the
		 * zombie window id and a create event for this new window is
		 * coming in. in this situation we have a double add from our
		 * point of view. we try to recover from it and be robust
		 * about it, by updating the existing entry
		 */
		logger.warn() << "double-add of window "
			<< win_dir->name() << ": updating existing entry\n";
		auto orig_entry = reinterpret_cast<WindowDirEntry*>(
			getDirEntry(win_dir->name())
		);
		orig_entry->updateAll();
		// delete the duplicate
		delete win_dir;
	}
	catch( ... )
	{
		delete win_dir;
		throw;
	}
}

void WindowsRootDir::updateProperty(const XWindow &win, Atom changed_atom)
{
	auto win_dir = getWindowDir(win);

	if( !win_dir )
	{
		return missingWindow("property update");
	}

	win_dir->update(changed_atom);
}

void WindowsRootDir::updateMappedState(const XWindow &win, const bool is_mapped)
{
	auto win_dir = getWindowDir(win);

	if( !win_dir )
	{
		return missingWindow("Mapping state update");
	}

	xwmfs::StdLogger::getInstance().info()
		<< "Mapped state for window " << win
		<< " changed to " << is_mapped << std::endl;

	win_dir->newMappedState(is_mapped);
}


void WindowsRootDir::updateParent(const XWindow &win)
{
	auto win_dir = getWindowDir(win);

	if( !win_dir )
	{
		return missingWindow("parent update");
	}

	xwmfs::StdLogger::getInstance().info()
		<< "New parent for " << win
		<< ": " << XWindow(win.getParent()) << std::endl;

	win_dir->newParent(XWindow(win.getParent()));
}

void WindowsRootDir::missingWindow(const std::string &action)
{
	xwmfs::StdLogger::getInstance().warn()
		<< "Window not found in hierarchy for: " << action
		<< std::endl;
}

} // end ns

