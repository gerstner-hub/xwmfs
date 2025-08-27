#pragma once

// libxpp
#include <xpp/RootWin.hxx>
#include <xpp/types.hxx>

// xwmfs
#include "main/UpdateableDir.hxx"

namespace xpp {
	class XWindow;
}

namespace xwmfs {

class WinManagerWindow;
class EventFile;

/**
 * \brief
 * 	A DirEntry that contains and manages global window manager properties
 * \details
 * 	This is a specialized DirEntry representing the window manager. It
 * 	contains a number of sub-entries that contain global window manager
 * 	properties and controls.
 **/
class WinManagerDirEntry :
		public UpdateableDir<WinManagerDirEntry> {
public: // functions

	explicit WinManagerDirEntry(WinManagerWindow &root_win);

	/**
	 * \brief
	 * 	Update window manager data denoted by \c changed_atom
	 **/
	void update(const xpp::AtomID changed_atom);

	/**
	 * \brief
	 * 	Reflect delection of the denoted \c deleted_atom
	 **/
	void delProp(const xpp::AtomID deleted_atom);

	/**
	 * \brief
	 * 	To be called when a window was created or destroyed
	 **/
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
	//! an event file from where programs can efficiently read individual
	//! window manager events
	EventFile *m_events = nullptr;
};

} // end ns
