#pragma once

// libxpp
#include <xpp/fwd.hxx>
#include <xpp/XWindow.hxx>

// xwmfs
#include "main/UpdatableDir.hxx"

namespace xwmfs {

class EventFile;
class WindowFileEntry;

/// A specialized DirEntry for window directories.
/**
 * This is a directory representing an X window. It contains a number of
 * sub-entries that contain the window-specific information and controls.
 **/
class WindowDirEntry :
		public UpdatableDir<WindowDirEntry> {
public: // functions

	/// Create a new window dir entry.
	/**
	 * \param[in] query_attrs
	 * 	Query some window parameters actively during construction
	 * 	instead of waiting for update events
	 **/
	explicit WindowDirEntry(const xpp::XWindow &win,
			const bool query_attrs = false);

	/// Update window data denoted by `changed_atom`.
	void update(const xpp::AtomID changed_atom) {
		propertyChanged(changed_atom, false);
	}

	void update(const EntrySpec &spec);

	void delProp(const xpp::AtomID deleted_atom) {
		propertyChanged(deleted_atom, true);
	}

	void delProp(const EntrySpec &spec);

	/// The window has been (un)mapped.
	void newMappedState(const bool mapped);

	/// The window geometry changed according to `event`.
	void newGeometry(const xpp::ConfigureEvent &event);

	/// The window's parent changed.
	void newParent(const xpp::XWindow &win);

	/// Updates all information stored in the window directory.
	/**
	 * This is effectivelly a poll of all information from the X server.
	 **/
	void updateAll();

protected: // functions

	void propertyChanged(const xpp::AtomID changed_atom,
			const bool is_delete);

	/// adds all directory file entries for the represented window
	void addEntries();

	bool markDeleted() override;

	SpecVector getSpecVector() const;

	void addSpecEntry(const EntrySpec &spec);

	void forwardEvent(const EntrySpec &changed_entry);

	std::string getCommandInfo();

	/// Adds/updates the window name of the window.
	void updateWindowName(FileEntry &entry);

	/// Adds/updates an entry for the desktop nr. the window is on.
	void updateDesktop(FileEntry &entry);

	/// Adds/updates an entry for the ID for the window.
	void updateId(FileEntry &entry);

	/// Adds/updates an entry for the PID of the window owner.
	void updatePID(FileEntry &entry);

	/// Updates an entry for the command line of a window.
	void updateCommand(FileEntry &entry);

	/// Updates an entry for the window's locale name.
	void updateLocale(FileEntry &entry);

	/// Updates an entry for the window's supported protocols.
	void updateProtocols(FileEntry &entry);

	/// Updates an entry for the window's client leader window.
	void updateClientLeader(FileEntry &entry);

	/// Updates an entry for the window's type.
	void updateWindowType(FileEntry &entry);

	/// Adds/updates an entry for the command control file of a window.
	void updateCommandControl(FileEntry &entry);

	/// Adds/updates an entry for the client machine a window is running on.
	void updateClientMachine(FileEntry &entry);

	/// Adds/udpates a list of all properties of the window.
	void updateProperties(FileEntry &entry);

	/// Adds/updates the window instance and class name.
	void updateClass(FileEntry &entry);

	/// Adds an entry for the ID of the parent window.
	void updateParent();

	/// Updates the geometry entry according to \c attrs.
	void updateGeometry(const xpp::XWindowAttrs &attrs);

	/// Actively query some attributes.
	void queryAttrs();

	/// Set some default attributes.
	void setDefaultAttrs();

protected: // data

	/// The window we're representing with this directory.
	xpp::XWindow m_win;

	/// event file which returns window-specific events.
	EventFile *m_events = nullptr;
	/// Contains the mapped state of this window.
	WindowFileEntry *m_mapped = nullptr;
	/// Contains the ID of the parent of this window.
	WindowFileEntry *m_parent = nullptr;
	/// Contains the geometry of this window.
	WindowFileEntry *m_geometry = nullptr;
};

} // end ns
