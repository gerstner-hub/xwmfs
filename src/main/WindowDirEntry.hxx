#ifndef XWMFS_WINDOWDIRENTRY_HXX
#define XWMFS_WINDOWDIRENTRY_HXX

// C++
#include <set>

// xwmfs
#include "fuse/DirEntry.hxx"
#include "x11/XWindow.hxx"
#include "main/UpdateableDir.hxx"

namespace xwmfs
{

class EventFile;
class WindowFileEntry;

/**
 * \brief
 * 	A specialized DirEntry for Window directories
 * \details
 * 	This is a directory representing an X window. It contains a number of
 * 	sub-entries that contain the window-specific information and controls
 * 	specific to this window.
 **/
class WindowDirEntry :
	public UpdateableDir<WindowDirEntry>
{
public: // functions

	/**
	 * \brief
	 * 	Create a new window dir entry
	 * \param[in] query_attrs
	 * 	If set then during construction some window parameters will be
	 * 	polled from the window, instead of waiting for update events
	 **/
	explicit WindowDirEntry(
		const XWindow &win,
		const bool query_attrs = false
	);

	/**
	 * \brief
	 * 	Update window data denoted by \c changed_atom
	 **/
	void update(Atom changed_atom) { propertyChanged(changed_atom, false); }

	void update(const EntrySpec &spec);

	void delProp(Atom deleted_atom) { propertyChanged(deleted_atom, true); }

	void delProp(const EntrySpec &spec);

	//! the window has been (un)mapped
	void newMappedState(const bool mapped);

	//! the window geometry changed according to \c event
	void newGeometry(const XConfigureEvent &event);

	//! the window's parent has been changed
	void newParent(const XWindow &win);

	/**
	 * \brief
	 * 	Updates all information stored in the window directory
	 * \details
	 * 	This is effectivelly a poll of all information from the X
	 * 	server
	 **/
	void updateAll();

protected: // functions

	void propertyChanged(Atom changed_atom, bool is_delete);

	//! adds all directory file entries for the represented window
	void addEntries();

	bool markDeleted() override;

	SpecVector getSpecVector() const;

	void addSpecEntry(
		const UpdateableDir<WindowDirEntry>::EntrySpec &spec
	);

	void forwardEvent(const EntrySpec &changed_entry);

	std::string getCommandInfo();

	//! adds/updates the window name of the window
	void updateWindowName(FileEntry &entry);

	//! adds/updates an entry for the desktop nr the window is on
	void updateDesktop(FileEntry &entry);

	//! adds/updates an entry for the ID for the window
	void updateId(FileEntry &entry);

	//! adds/updates an entry for the PID of the window owner
	void updatePID(FileEntry &entry);

	//! updates an entry for the command line of a window
	void updateCommand(FileEntry &entry);

	//! updates an entry for the window's locale name
	void updateLocale(FileEntry &entry);

	//! updates an entry for the window's supported protocols
	void updateProtocols(FileEntry &entry);

	//! updates an entry for the window's client leader window
	void updateClientLeader(FileEntry &entry);

	//! updates an entry for the window's type
	void updateWindowType(FileEntry &entry);

	//! adds/updates an entry for the command control file of a window
	void updateCommandControl(FileEntry &entry);

	//! adds/updates an entry for the client machine a window is running on
	void updateClientMachine(FileEntry &entry);

	//! adds/udpates a list of all properties of the window
	void updateProperties(FileEntry &entry);

	//! adds/updates the window instance and class name
	void updateClass(FileEntry &entry);

	//! Adds an entry for the ID of the parent window
	void updateParent();

	//! Updates the geometry entry according to \c attrs
	void updateGeometry(const XWindowAttrs &attrs);

	//! Actively query some attributes
	void queryAttrs();

	//! Set some default attributes
	void setDefaultAttrs();

protected: // data

	//! the window we're representing with this directory
	XWindow m_win;

	//! an event file from which programs can efficiently read individual
	//! window events
	EventFile *m_events = nullptr;
	//! contains the mapped state of this window
	WindowFileEntry *m_mapped = nullptr;
	//! contains the ID of the parent of this window
	WindowFileEntry *m_parent = nullptr;
	//! contains the geometry of this window
	WindowFileEntry *m_geometry = nullptr;
};

} // end ns

#endif // inc. guard

