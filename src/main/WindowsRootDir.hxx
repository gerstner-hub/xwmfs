#pragma once

// libcosmos
#include "cosmos/utils.hxx"

// libxpp
#include <xpp/fwd.hxx>

// xwmfs
#include "fuse/DirEntry.hxx"

namespace xwmfs {

class WindowDirEntry;

/// Represents the "windows" root directory.
/**
 * All direct children of the root window will have a directory entry named
 * after the ID of the respective window in this root directory.
 **/
class WindowsRootDir :
		public DirEntry {
public: // types

	using InitialPopulation = cosmos::NamedBool<struct initial_pop_t, false>;
	using IsRootWin = cosmos::NamedBool<struct is_root_win_t, false>;

public: // functions

	WindowsRootDir();

	/// Removes the sub-directory matching the given window.
	/**
	 * It is possible that the given window isn't existing in the
	 * filesystem in which case the event should be ignored.
	 *
	 * Exceptions are handled by the caller.
	 **/
	void removeWindow(const xpp::XWindow &win);

	/// Adds the given window into the correct place in the hierarchy.
	/**
	 * This function is called upon window create events.
	 *
	 * \param[in] initial
	 * 	If set then this is the initial population of windows and thus
	 * 	a full query of properties should be made.
	 * \param[in] is_root_win
	 * 	If set then `win` refers to the root window. For the root
	 * 	window no property updates are processed.
	 **/
	void addWindow(const xpp::XWindow &win,
			const InitialPopulation initial = InitialPopulation{false},
			const IsRootWin is_root_win = IsRootWin{false}
	);

	/// Returns a pointer to the file system entry corresponding to `win`.
	/**
	 * \return
	 * 	The matching pointer or nullptr if not found.
	 **/
	WindowDirEntry* getWindowDir(const xpp::XWindow &win);

	/// Called if a window's property in the file system is to be updated.
	/**
	 * \param[in] win
	 * 	The window that changed.
	 * \param[in] changed_atom
	 *	The atom at the window that changed.
	 **/
	void updateProperty(const xpp::XWindow &win,
			const xpp::AtomID changed_atom);

	/// Called if a window's property in the file system is to be deleted.
	/**
	 * \param[in] win
	 * 	The window that lost a property.
	 * \param[in] deleted_atom
	 * 	The atom at the window that was deleted.
	 **/
	void deleteProperty(const xpp::XWindow &win,
			const xpp::AtomID deleted_atom);

	/// Called if a window's geometry has changed.
	/**
	 * \param[in] win
	 * 	The window that changed
	 **/
	void updateGeometry(const xpp::XWindow &win,
			const xpp::ConfigureEvent &event);

	/// Called if the mapped state of a window is to be updated.
	/**
	 * \param[in] win
	 * 	The window that was (un)mapped
	 * \param[in] is_mapped
	 * 	Whether the window is now mapped or unmapped
	 **/
	void updateMappedState(const xpp::XWindow &win, const bool is_mapped);

	/// Updates the parent of the given window `win`.
	/**
	 * The new parent needs to be available via win.getParent().
	 *
	 * The window, if currently existing in the hierarchy, will be moved
	 * to the new parent.
	 **/
	void updateParent(const xpp::XWindow &win);

protected: // functions

	void missingWindow(const xpp::XWindow &win,
			const std::string &action);
};

} // end ns
