#ifndef XWMFS_ROOTWINDOW_HXX
#define XWMFS_ROOTWINDOW_HXX

#include <sstream>
#include <vector>

// display representation class
#include "x11/XDisplay.hxx"
// window representation class
#include "x11/XWindow.hxx"

namespace xwmfs
{

/**
 * \brief
 * 	This class is a special window, the root window
 * \details
 * 	The root window contains information about all other windows opened
 * 	and it has attached information about the window manager.
 *
 * 	Thus appropriate operations are added to the common XWindow in this
 * 	class.
 **/
class RootWin :
	public XWindow
{
public: // types

	//! specialized exception for root window query errors
	class QueryError :
		public xwmfs::Exception
	{
	public: // functions

		QueryError(const std::string &s) : Exception(s) { }

		XWMFS_EXCEPTION_IMPL;
	};

	/**
	 * \brief
	 * 	Enum for differentiating different window manager
	 * 	implementations
	 * \details
	 * 	This is for future use if we need to adjust our logic to the
	 * 	actual window manager encountered.
	 **/
	enum class WindowManager
	{
		//! the fluxbox window manager
		FLUXBOX,
		//! the i3 tiling window manager
		I3,
		//! unknown window manager encountered
		UNKNOWN
	};

public: // functions

	/**
	 * \brief
	 * 	Creates the root window and queries associated properties via
	 * 	getInfo()
	 **/
	RootWin();

	/**
	 * \brief
	 * 	Checks for presence of compatible WM and retrieves information
	 * 	about it
	 **/
	void getInfo();

	//! returns whether a window manager name was found
	bool hasWM_Name() const { return !m_wm_name.empty(); }
	
	//! returns whether a window manager PID was found
	bool hasWM_Pid() const { return m_wm_pid != -1; }

	//! returns whether a window manager class was found
	bool hasWM_Class() const { return m_wm_class.valid(); }

	//! returns whether a window manager show the desktop mode was found
	bool hasWM_ShowDesktopMode() const { return m_wm_showing_desktop != -1; }

	//! returns whether the currently active desktop was found
	bool hasWM_ActiveDesktop() const { return m_wm_active_desktop != -1; }
	
	//! returns whether the currently active desktop was found
	bool hasWM_ActiveWindow() const { return m_wm_active_window != 0; }

	//! returns whether the current number of desktops was found
	bool hasWM_NumDesktops() const { return m_wm_num_desktops != -1; }

	//! returns the window manager name, if found
	const char* getWM_Name() const { return m_wm_name.data(); }

	//! returns the window manager PID, if found
	int getWM_Pid() const { return m_wm_pid; }

	//! returns the window manager class, if found
	char* getWM_Class() const { return m_wm_class.get().data; }

	//! returns the show the desktop mode, if found
	bool getWM_ShowDesktopMode() const { return m_wm_showing_desktop == 1; }

	//! returns the currently active desktop, if found
	int getWM_ActiveDesktop() const { return m_wm_active_desktop; }

	//! changes the active desktop
	void setWM_ActiveDesktop(const int &num);

	//! returns the number of desktops determined
	int getWM_NumDesktops() const { return m_wm_num_desktops; }

	//! sets the number of desktops
	void setWM_NumDesktops(const int &num);

	//! returns the window manager type running
	WindowManager getWM_Type() const { return m_wm_type; }

	//! returns the currently focused window, if any / supported
	Window getWM_ActiveWindow() const { return m_wm_active_window; }

	//! requests to change the focus to the given window
	void setWM_ActiveWindow(const XWindow &win);

	//! Returns the list of windows managed by the window manager
	const std::vector<XWindow>& getWindowList() const
	{ return m_windows; }

	void updateShowingDesktop();
	void updateActiveDesktop();
	void updateNumberOfDesktops();
	void updateActiveWindow();

protected: // functions

	/**
	 * \brief
	 * 	Checks for existance of WM child window and correct setup of
	 * 	it
	 * \details
	 * 	Sets m_ewmh_child to the child window set by the window
	 * 	manager
	 * \postcondition
	 * 	m_ewmh_child is set to the valid child window of the
	 * 	running WM
	 * \exception
	 * 	WMQueryError if WM couldn't be queried or is not running
	 **/
	void queryWMWindow();

	/**
	 * \brief
	 *	Queries direct properties of the WM like it's name, class, pid
	 *	and show desktop mode
	 **/
	void queryBasicWMProperties();

	//! returns the window manager enum matching the given wm name
	WindowManager detectWM(const std::string &name);

	//! determines the PID of the window manager
	void queryPID();

	/**
	 * \brief
	 * 	Queries all existing windows from the WM and stores them in
	 * 	m_windows
	 **/
	void queryWindows();

private: // data

	//! \brief
	//! if valid then this is the child window associated with EWMH
	//! compatible WM
	XWindow m_ewmh_child;

	//! queried name of the window manager in UTF8 (or NULL if N/A)
	std::string m_wm_name;
	//! the process ID of the window manager (or -1 if N/A)
	int m_wm_pid = -1;
	//! queried class of the window manager in UTF8 (or NULL if N/A)
	xwmfs::Property<xwmfs::utf8_string> m_wm_class;
	//! \brief
	//! integer specifying whether currently the "show desktop mode" is
	//! active (or -1 if N/A)
	int m_wm_showing_desktop = -1;

	//! An array of all windows existing, in undefined order
	std::vector<XWindow> m_windows;

	//! the concrete type of window manager encountered
	WindowManager m_wm_type = WindowManager::UNKNOWN;

	//! number of available desktops
	int m_wm_num_desktops = -1;

	//! the currently active/shown desktop nr.
	int m_wm_active_desktop = -1;

	//! the window that currently has focus
	Window m_wm_active_window = 0;

	//! names of the desktops 0 ... m_wm_num_desktops
	std::vector<std::string> m_wm_desktop_names;
};

} // end ns

#endif // inc. guard

