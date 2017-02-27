#ifndef XWMFS_XWMFS_HXX
#define XWMFS_XWMFS_HXX

#include <sys/select.h>

#include "common/Thread.hxx"
#include "x11/RootWin.hxx"
#include "fuse/RootEntry.hxx"
#include "main/Options.hxx"
#include "main/StdLogger.hxx"

namespace xwmfs
{

class WinManagerDirEntry;

/**
 * \brief
 * 	The main application class that provides XWMFS functionality
 * \details
 * 	This is the connector class between the X11 and FUSE related XWMFS
 * 	parts. It keeps the window manager information on the one hand and the
 * 	FUSE file system representation on the other hand.
 *
 * 	This is a singleton class such that it can be accessed globally from
 * 	anyhwere. Any application-global data is kept here.
 *
 * 	This class also runs its own thread that is responsible for dealing
 * 	with events dispatched from Xlib to us. This allows us to update the
 * 	file system structure whenever relevant window manager information
 * 	changes.
 **/
class Xwmfs :
	protected IThreadEntry
{
public: // functions

	/**
	 * \brief
	 * 	An early initialization function that needs to be called from
	 * 	the main() function early on
	 **/
	static void early_init();

	/**
	 * \brief
	 *	This function is called by FUSE for initialization
	 * \details
	 * 	This function is only called from within FUSE initialization
	 * 	and thus can't throw any exceptions.
	 * \return
	 *	EXIT_SUCCESS if initialization went well, else otherwise.
	 **/
	int init();

	/**
	 * \brief
	 *	This function is called by FUSE for cleanup
	 **/
	void exit();

	~Xwmfs();

	//! returns the global XWMFS instance
	static xwmfs::Xwmfs& getInstance()
	{
		static Xwmfs w;

		return w;
	}

	//! Returns the root window held in the Xwmfs
	xwmfs::RootWin& getRootWin() { return m_root_win; }

	//! returns the file system structure root entry
	xwmfs::RootEntry& getFS() { return m_fs_root; }

	//! returns the options in effect for Xwmfs
	xwmfs::Options& getOptions() { return m_opts; }

	//! returns the umask of the current process
	static mode_t getUmask() { return m_umask; }

	//! returns the current time (update for each new X event)
	time_t getCurrentTime() const { return m_current_time; }

protected: // functions

	/**
	 * \brief
	 * 	Called from Xlib on protocol errors
	 * \details
	 * 	If we don't overwrite this handler then the default handler
	 * 	from xlib will call exit() in situations that can happen in
	 * 	normal operation to us like race conditions e.g. a window
	 * 	property updated but then the window also is destroyed but we
	 * 	still attempt to get the properties from the already dead
	 * 	window.
	 *
	 * 	For being able to recover such situations we overwrite the
	 * 	error handler and simply print out information to stdout
	 * 	without exiting.
	 **/
	static int XErrorHandler(Display *disp, XErrorEvent *error);

	/**
	 * \brief
	 * 	Called from Xlib on fatal error conditions
	 * \details
	 * 	This is called if fatal errors occur like the X connection
	 * 	being lost. It's supposed not to return, otherwise Xlib calls
	 * 	exit()
	 **/
	static int XIOErrorHandler(Display *disp);

	//! working loop for the event handling thread
	void threadEntry(const Thread &t) override;

	/**
	 * \brief
	 * 	Handles a single X11 event received by the x11 event thread
	 **/
	void handleEvent(const XEvent &ev);

	/**
	 * \brief
	 * 	Handles a window CreateNotify event
	 **/
	void handleCreateEvent(const XEvent &ev);

	//! called from m_ev_thread if a window is to be removed from the FS
	void removeWindow(const XWindow &win);

	//! called from m_ev_thread if a window is to be added to the FS
	void addWindow(const XWindow &win);

	/**
	 * \brief
	 *	called from m_ev_thread if a window in the file system is to
	 *	be updated
	 * \param[in] win
	 * 	The window that changed
	 * \param[in] changed_atom
	 * 	The atom at the window that changed
	 **/
	void updateWindow(const XWindow &win, Atom changed_atom);

	/**
	 * \brief
	 * 	called from m_ev_thread if the root window in the file system
	 * 	is to be updated
	 * \param[in] changed_atom
	 * 	The atom at the root window that changed
	 **/
	void updateRootWindow(Atom changed_atom);

private: // data

	//! \brief
	//! The gathered window manager information for the current X
	//! display (X11 part of XWMFS)
	xwmfs::RootWin m_root_win;

	//! \brief
	//! The file system root entry that composes the complete file system
	//! (FUSE part of XWMFS)
	xwmfs::RootEntry m_fs_root;

	//! Thread evaluating X11 events and updating m_wmi, m_fs_root
	xwmfs::Thread m_ev_thread;

	//! options for the current instance
	xwmfs::Options &m_opts;

	// this is the fd for the connection to the display
	int m_dis_fd = -1;
	// wakeup pipe read and write end
	int m_wakeup_pipe[2];

	//! file descriptors to monitor for events
	fd_set m_select_set;

	//! the time of the last event that might lead to creating new file
	//! system objects
	time_t m_current_time = 0;

	//! directory node containing all windows
	DirEntry *m_win_dir = nullptr;
	//! directory node containing global wm information
	WinManagerDirEntry *m_wm_dir = nullptr;

	//! the active umask of the current process
	static mode_t m_umask;

private: // functions

	//! private constructor to enforce singleton pattern
	Xwmfs();

	//! deleted copy constructor to enforce singleton pattern
	Xwmfs(const Xwmfs &w) = delete;

	/**
	 * \brief
	 * 	Creates the initial filesystem
	 **/
	void createFS();

	//! print application state for debugging purposes
	void printWMInfo();

	//! updates m_current_time with the current time :-)
	void updateTime();
};

} // end ns

#endif // inc. guard

