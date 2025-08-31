#pragma once

// libxpp
#include <xpp/fwd.hxx>
#include <xpp/types.hxx>

// xwmfs
#include "main/UpdateableDir.hxx"

namespace xwmfs {

class WinManagerWindow;
class EventFile;

/// A DirEntry that contains and manages global window manager properties.
/**
 * This is a specialized DirEntry representing the window manager. It contains
 * a number of sub-entries that contain global window manager properties and
 * controls.
 **/
class WinManagerDirEntry :
		public UpdateableDir<WinManagerDirEntry> {
public: // functions

	explicit WinManagerDirEntry(WinManagerWindow &root_win);

	/// Update window manager data denoted by `changed_atom`.
	void update(const xpp::AtomID changed_atom);

	/// Reflect deletion of the denoted `deleted_atom` in the FS.
	void delProp(const xpp::AtomID deleted_atom);

	/// To be called when a window was created or destroyed.
	void windowLifecycleEvent(
		const xpp::XWindow &win,
		const bool created_else_destroyed
	);

protected: // functions

	void addEntries();

	void addSpecEntry(const UpdateableDir<WinManagerDirEntry>::EntrySpec &spec);

	SpecVector getSpecVector() const;

	void forwardEvent(const EntrySpec &changed_entry);

	void updateNumberOfDesktops(FileEntry &entry);
	void updateDesktopNames(FileEntry &entry);
	void updateActiveDesktop(FileEntry &entry);
	void updateActiveWindow(FileEntry &entry);
	void updateShowDesktopMode(FileEntry &entry);
	void updateName(FileEntry &entry);
	void updateClass(FileEntry &entry);

protected: // data

	WinManagerWindow &m_root_win;
	/// File for reading window manager wide events.
	EventFile *m_events = nullptr;
};

} // end ns
