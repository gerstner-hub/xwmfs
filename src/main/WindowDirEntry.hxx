#ifndef XWMFS_WINDOWDIRENTRY_HXX
#define XWMFS_WINDOWDIRENTRY_HXX

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
	void update(Atom changed_atom);

	//! the window has been (un)mapped
	void newMappedState(const bool mapped);

protected: // functions

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

	//! Adds an entry for the command control file of a window
	void updateCommandControl(FileEntry &entry);

	//! Adds an entry for the client machine a window is running on
	void updateClientMachine(FileEntry &entry);

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
};

} // end ns

#endif // inc. guard

