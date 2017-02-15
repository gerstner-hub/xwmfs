#ifndef XWMFS_WINMANAGERDIRENTRY_HXX
#define XWMFS_WINMANAGERDIRENTRY_HXX

// xwmfs
#include "x11/XAtom.hxx"
#include "main/UpdateableDir.hxx"

namespace xwmfs
{

class RootWin;

/**
 * \brief
 * 	A DirEntry that contains and manages global window manager properties
 * \details
 * 	This is a specialized DirEntry representing the window manager. It
 * 	contains a number of sub-entries that contain global window manager
 * 	properties and controls.
 **/
class WinManagerDirEntry : 
	public UpdateableDir<WinManagerDirEntry>
{
public: // functions

	explicit WinManagerDirEntry(RootWin &root_win);

	/**
	 * \brief
	 * 	Update window manager data denoted by \c changed_atom
	 **/
	void update(const Atom changed_atom);

protected: // functions

	void addEntries();

	void addSpecEntry(
		const UpdateableDir<WinManagerDirEntry>::EntrySpec &spec
	);

	SpecVector getSpecVector() const;

	void updateNumberOfDesktops(FileEntry &entry);
	void updateActiveDesktop(FileEntry &entry);
	void updateActiveWindow(FileEntry &entry);
	void updateShowDesktopMode(FileEntry &entry);
	void updateName(FileEntry &entry);
	void updateClass(FileEntry &entry);

protected: // data

	RootWin &m_root_win;
};

} // end ns

#endif // inc. guard

