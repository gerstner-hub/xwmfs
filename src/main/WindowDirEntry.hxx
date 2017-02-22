#ifndef XWMFS_WINDOWDIRENTRY_HXX
#define XWMFS_WINDOWDIRENTRY_HXX

// xwmfs
#include "fuse/DirEntry.hxx"
#include "x11/XWindow.hxx"
#include "main/UpdateableDir.hxx"

namespace xwmfs
{


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

	explicit WindowDirEntry(const XWindow &win);

	/**
	 * \brief
	 * 	Update window data denoted by \c changed_atom
	 **/
	void update(Atom changed_atom);

protected: // functions

	//! adds all directory file entries for the represented window
	void addEntries();

	SpecVector getSpecVector() const;
	
	void addSpecEntry(
		const UpdateableDir<WindowDirEntry>::EntrySpec &spec
	);

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
	
protected: // data

	//! the window we're representing with this directory
	XWindow m_win;
};

} // end ns

#endif // inc. guard

