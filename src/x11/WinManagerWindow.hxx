#pragma once

// C++
#include <optional>
#include <string_view>

// libxpp
#include <xpp/Property.hxx>
#include <xpp/RootWin.hxx>

namespace xpp {
	class XWindow;
}

namespace xwmfs {

/// Wrapper around xpp::RootWin with added window manager logic.
class WinManagerWindow :
		public xpp::RootWin {
public: // types

	/// Enum for differentiating different window manager implementations.
	/**
	 * This is for future use if we need to adjust our logic to the actual
	 * window manager encountered.
	 **/
	enum class WindowManager {
		/// the Fluxbox window manager.
		FLUXBOX,
		/// the i3 tiling window manager.
		I3,
		/// unknown window manager encountered.
		UNKNOWN
	};

public: // functions

	/// Accesses the root window and queries associated window manager properties.
	explicit WinManagerWindow(xpp::XDisplay &display);

	/// Checks for presence of compatible WM and retrieves information about it.
	void getInfo();

	/// Asks the window manager to move the given window to the given desktop nr.
	void moveToDesktop(const xpp::XWindow &win, const int desktop);


	/// Returns whether the currently active desktop was found.
	bool hasActiveDesktop() const { return m_wm_active_desktop != std::nullopt; }

	/// Returns the currently active desktop, if found.
	int getActiveDesktop() const { return *m_wm_active_desktop; }

	/// Asks the window manager to change the active desktop.
	void setActiveDesktop(const int num);

	/// Fetches the currently active desktop from the window manager.
	void fetchActiveDesktop();


	/// Returns whether the current number of desktops was found.
	bool hasNumDesktops() const { return m_wm_num_desktops != std::nullopt; }

	/// Returns the number of desktops determined.
	int getNumDesktops() const { return *m_wm_num_desktops; }

	/// Asks the window manager to change the number of desktops.
	void setNumDesktops(const int num);

	/// Fetches the current number of desktops from the window manager.
	void fetchNumDesktops();


	/// Returns whether the currently active window was found.
	bool hasActiveWindow() const { return m_wm_active_window != std::nullopt; }

	/// Returns the currently focused window, if any / supported.
	xpp::WinID getActiveWindow() const { return *m_wm_active_window; }

	/// Requests to change the focus to the given window.
	void setActiveWindow(const xpp::XWindow &win);

	/// Fetches the currently active window from the window manager.
	void fetchActiveWindow();


	/// Returns whether a window manager "show the desktop mode" was found.
	bool hasShowDesktopMode() const { return m_wm_showing_desktop != std::nullopt; }

	/// Returns the show the desktop mode, if found.
	bool getShowDesktopMode() const { return *m_wm_showing_desktop == 1; }

	/// Fetches the current "showing desktop mode" setting from the WM.
	void fetchShowDesktopMode();


	/// Returns whether a window manager name was found.
	bool hasWMName() const { return !m_wm_name.empty(); }

	/// Returns the window manager name, if found.
	const std::string& getWMName() const { return m_wm_name; }


	/// Returns whether a window manager PID was found.
	bool hasWMPid() const { return m_wm_pid != std::nullopt; }

	/// Returns the window manager PID.
	int getWMPid() const { return *m_wm_pid; }


	/// Returns whether a window manager class was found.
	bool hasWMClass() const { return m_wm_class.valid(); }

	/// Returns the window manager class, if found.
	std::string_view getWMClass() const { return m_wm_class.get().str; }


	/// Returns the vector of known desktop names in order of occurrence.
	const auto& getDesktopNames() const { return m_wm_desktop_names; }

	/// Fetches the current desktop names from the WM.
	void fetchDesktopNames();

protected: // functions

	/// Checks for existance of WM child window and correct setup of it.
	/**
	 * Sets m_ewmh_child to the child window set by the window manager
	 *
	 * \postcondition
	 * 	m_ewmh_child is set to the valid child window of the
	 * 	running WM
	 **/
	void queryWMWindow();

	/**
	 * \brief
	 *	Queries direct properties of the WM like its name, class, pid
	 *	and show desktop mode.
	 **/
	void queryBasicWMProperties();

	/// returns the window manager enum matching the given wm name.
	WindowManager detectWM(const std::string &name);

	/// determines the PID of the window manager.
	void queryPID();

protected: // data

	/// Contains the child window associated with EWMH compatible WM, if applicable.
	xpp::XWindow m_ewmh_child;

	/// the concrete type of window manager encountered.
	WindowManager m_wm_type = WindowManager::UNKNOWN;

	/// Number of available desktops.
	std::optional<int> m_wm_num_desktops;

	/// The currently active/shown desktop nr.
	std::optional<int> m_wm_active_desktop;

	/// The process ID of the window manager.
	std::optional<int> m_wm_pid;

	/// Whether currently the "show desktop mode" is active.
	std::optional<int> m_wm_showing_desktop;

	/// The window that currently has focus.
	std::optional<xpp::WinID> m_wm_active_window;

	/// Name of the window manager in UTF8 (or empty if N/A).
	std::string m_wm_name;

	/// Class of the window manager in UTF8 (or invalid if N/A).
	xpp::Property<xpp::utf8_string> m_wm_class;

	/// Names of the desktops 0 ... m_wm_num_desktops
	std::vector<std::string> m_wm_desktop_names;
};

} // end ns
