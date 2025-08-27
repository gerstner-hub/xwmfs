#pragma once

// C++
#include <map>

// xwmfs
#include "fuse/DirEntry.hxx"

namespace xpp {
	class XWindow;
}

namespace xwmfs {

class WindowDirEntry;

/**
 * \brief
 * 	Represents the "windows" root directory that contains all direct
 * 	children of the root window
 * \details
 * 	All direct childs of the root window will have a directory entry named
 * 	after the ID of the respective window in this root directory.
 **/
class WindowsRootDir :
		public DirEntry {
public: // functions

	WindowsRootDir();

	/**
	 * \brief
	 * 	Removes the sub-directory matching the given window
	 **/
	void removeWindow(const xpp::XWindow &win);

	/**
	 * \brief
	 * 	Adds the given window into the correct place in the hierarchy
	 * \param[in] initial
	 * 	If set then this is the initial population of windows and thus
	 * 	a full query of properties should be made
	 * \param[in] is_root_win
	 * 	If set then \c win refers to the root window. This triggers
	 * 	some special logic
	 **/
	void addWindow(const xpp::XWindow &win, const bool initial = false,
			const bool is_root_win = false);

	/**
	 * \brief
	 * 	Returns a pointer to the file system entry corresponding to \c
	 * 	win
	 * \returns
	 * 	The matching pointer or nullptr if not found
	 **/
	WindowDirEntry* getWindowDir(const xpp::XWindow &win);

	/**
	 * \brief
	 *	Called if a window's property in the file system is to be
	 *	updated
	 * \param[in] win
	 * 	The window that changed
	 * \param[in] changed_atom
	 * 	The atom at the window that changed
	 **/
	void updateProperty(const xpp::XWindow &win, const xpp::AtomID changed_atom);

	/**
	 * \brief
	 * 	Called if a window's property in the file system is to be
	 * 	deleted
	 * \param[in] win
	 * 	The window that lost a property
	 * \param[in] deleted_atom
	 * 	The atom at the window that was deleted
	 **/
	void deleteProperty(const xpp::XWindow &win, const xpp::AtomID deleted_atom);

	/**
	 * \brief
	 * 	Called if a window's geometry has changed
	 * \param[in] win
	 * 	The window that changed
	 **/
	void updateGeometry(const xpp::XWindow &win, const xpp::ConfigureEvent &event);

	/**
	 * \brief
	 * 	called if the mapped state of a window is to be updated
	 * \param[in] win
	 * 	The window that was (un)mapped
	 * \param[in] is_mapped
	 * 	Whether the window is now mapped or unmapped
	 **/
	void updateMappedState(const xpp::XWindow &win, const bool is_mapped);

	/**
	 * \brief
	 * 	Updates the parent of the given window \c win
	 * \details
	 * 	The new parent needs to be available via win.getParent().
	 *
	 * 	The window, if currently existing in the hierarchy, will be
	 * 	moved to the new parent.
	 **/
	void updateParent(const xpp::XWindow &win);

protected: // functions

	void missingWindow(const xpp::XWindow &win, const std::string &action);

protected: // data

};

} // end ns
