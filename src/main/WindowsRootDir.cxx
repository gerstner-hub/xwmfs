// libxpp
#include <xpp/formatting.hxx>
#include <xpp/XWindow.hxx>

// xwmfs
#include "main/logger.hxx"
#include "main/WindowDirEntry.hxx"
#include "main/WindowsRootDir.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs {

WindowsRootDir::WindowsRootDir() :
		DirEntry{"windows"} {
}

void WindowsRootDir::removeWindow(const xpp::XWindow &win) {
	removeEntry(xpp::to_string(win.id()));
}

WindowDirEntry* WindowsRootDir::getWindowDir(const xpp::XWindow &win) {
	auto win_dir = dynamic_cast<WindowDirEntry*>(
		getDirEntry(xpp::to_string(win.id()))
	);

	return win_dir;
}

void WindowsRootDir::addWindow(const xpp::XWindow &win,
		const InitialPopulation initial, const IsRootWin is_root_win) {
	if (!is_root_win) {
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
	Xwmfs::getInstance().getDisplay().sync();

	auto win_dir = new xwmfs::WindowDirEntry{win, initial ? true : false};

	try {
		// the window directories are named after their IDs
		addEntry(win_dir, DirEntry::InheritTime{false});
		logger->debug() << "Added window "
			<< xpp::to_string(win.id()) << "\n";
	} catch (const DirEntry::DoubleAddError &) {
		/*
		 * This situation happens sometimes e.g. on i3 window manager.
		 * a window is destroyed but some kind of zombie entry remains
		 * in the client list. If xwmfs starts up in this situation
		 * then it will populate this zombie window in the file
		 * system, however all operations on it will fail, thus many
		 * directory nodes will be missing.
		 *
		 * When a new window is created then i3 seems to recycle the
		 * zombie window id and a create event for this new window is
		 * coming in. In this situation we have a double add from our
		 * point of view. We try to recover from it and be robust
		 * about it, by updating the existing entry
		 */
		logger->warn() << "double-add of window "
			<< win_dir->name() << ": updating existing entry\n";
		auto orig_entry = dynamic_cast<WindowDirEntry*>(
			getDirEntry(win_dir->name())
		);

		if (orig_entry) {
			orig_entry->updateAll();
		} else {
			logger->error() << "double-add of window, but existing entry is not a WindowDirEntry?!\n";
		}
		// delete the duplicate
		delete win_dir;
	} catch (...) {
		delete win_dir;
		throw;
	}
}

void WindowsRootDir::updateProperty(const xpp::XWindow &win, const xpp::AtomID changed_atom) {
	auto win_dir = getWindowDir(win);

	if (!win_dir) {
		return missingWindow(win, "property update");
	}

	win_dir->update(changed_atom);
}

void WindowsRootDir::deleteProperty(const xpp::XWindow &win, const xpp::AtomID deleted_atom) {
	auto win_dir = getWindowDir(win);

	if (!win_dir) {
		return missingWindow(win, "property delete");
	}

	win_dir->delProp(deleted_atom);
}

void WindowsRootDir::updateGeometry(const xpp::XWindow &win, const xpp::ConfigureEvent &event) {
	auto win_dir = getWindowDir(win);

	if (!win_dir) {
		return missingWindow(win, "geometry update");
	}

	win_dir->newGeometry(event);
}

void WindowsRootDir::updateMappedState(const xpp::XWindow &win, const bool is_mapped) {
	auto win_dir = getWindowDir(win);

	if (!win_dir) {
		return missingWindow(win, "Mapping state update");
	}

	logger->info() << "Mapped state for window " << xpp::to_string(win.id())
		<< " changed to " << is_mapped << std::endl;

	win_dir->newMappedState(is_mapped);
}


void WindowsRootDir::updateParent(const xpp::XWindow &win) {
	auto win_dir = getWindowDir(win);

	if (!win_dir) {
		return missingWindow(win, "parent update");
	}

	logger->info()
		<< "New parent for " << xpp::to_string(win.id())
		<< ": " << xpp::XWindow{win.getParent()} << std::endl;

	win_dir->newParent(xpp::XWindow{win.getParent()});
}

void WindowsRootDir::missingWindow(const xpp::XWindow &win, const std::string &action) {
	logger->warn() << "Window " << win << " not found in hierarchy for: " << action << "\n";
}

} // end ns
