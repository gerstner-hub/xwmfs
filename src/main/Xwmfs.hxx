#ifndef XWMFS_XWMFS_HXX
#define XWMFS_XWMFS_HXX

// POSIX
#include <sys/select.h>

// C++
#include <map>
#include <set>

// Xwmfs
#include "common/Thread.hxx"
#include "x11/RootWin.hxx"
#include "fuse/RootEntry.hxx"
#include "main/Options.hxx"
#include "main/StdLogger.hxx"

namespace xwmfs
{

class WinManagerDirEntry;
class WindowDirEntry;
class WindowsRootDir;
class EventFile;

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

	/**
	 * \brief
	 * 	Registers blocking call situation
	 * \details
	 * 	The calling thread will be associated with the given file
	 * 	object. The callee will make sure that fuse signals to abort
	 * 	the blocking request will be caught and the blocking call
	 * 	woken up.
	 *
	 * 	In any case the caller must call unregisterBlockingCall()
	 * 	after the blocking call is over, whether aborted or not.
	 **/
	bool registerBlockingCall(EventFile *f);

	/**
	 * \brief
	 * 	Unregisters a previously registered blocking call situation
	 **/
	void unregisterBlockingCall();

protected: // functions

	friend void fuseAbortSignal(int);

	/**
	 * \brief
	 * 	Called from an asynchronous signal handler in case a blocking
	 * 	call shall be aborted for the calling thread
	 **/
	void abortBlockingCall(const bool all);

	/**
	 * \brief
	 * 	Abort a blocking call for the given thread, called
	 * 	synchronously from the event handling thread
	 **/
	void abortBlockingCall(pthread_t thread);

	/**
	 * \brief
	 * 	Called synchronously from the event calling thread to abort
	 * 	all pending blocking calls
	 **/
	void abortAllBlockingCalls();


	/**
	 * \brief
	 * 	Read an available message from the abort pipe
	 **/
	void readAbortPipe();

	/**
	 * \brief
	 * 	Sets up any asynchronous signal handlers we need for aborting
	 * 	blocking calls
	 **/
	void setupAbortSignals(const bool on_off);

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
	 * 	Reads and processes all events that can currently be read
	 * 	without blocking
	 **/
	void handlePendingEvents();

	/**
	 * \brief
	 * 	Handles a single X11 event received by the x11 event thread
	 **/
	void handleEvent(const XEvent &ev);

	/**
	 * \brief
	 * 	Handles a window CreateNotify event
	 * \return
	 * 	Whether the window has been added to the file system or not
	 **/
	bool handleCreateEvent(const XCreateWindowEvent &ev);

	/**
	 * \brief
	 * 	Returns whether the given CreateNotify event refers to a
	 * 	pseudo window
	 **/
	bool isPseudoWindow(const XCreateWindowEvent &ev) const;

	bool isIgnored(const XWindow &win) const
	{
		return m_ignored_windows.find(win.id()) != m_ignored_windows.end();
	}

private: // types

	typedef std::map<pthread_t, EventFile*> BlockingCallMap;
	typedef std::map<int, struct sigaction> SignalHandlerMap;

	//! different abort signal contexts
	enum class AbortType
	{
		//! abort just a single ongoing call in the associated thread
		CALL,
		//! abort all ongoing blocking calls to prepare for shutdown
		SHUTDOWN
	};

	struct AbortMsg
	{
		AbortType type;
		pthread_t thread = 0;
	};

	typedef std::set<Window> WindowSet;

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

	//! this is the fd for the connection to the display
	int m_dis_fd = -1;
	//! wakeup pipe read and write end
	int m_wakeup_pipe[2];

	//! file descriptors to monitor for events
	fd_set m_select_set;

	XEvent m_ev;
	Display *m_display = nullptr;

	//! the time of the last event that might lead to creating new file
	//! system objects
	time_t m_current_time = 0;

	//! directory node containing all windows
	WindowsRootDir *m_win_dir = nullptr;
	//! directory node containing global wm information
	WinManagerDirEntry *m_wm_dir = nullptr;

	//! abort pipe to signal abort requests for a specific thread
	int m_abort_pipe[2];
	//! a mapping of active blocking threads and their associated files
	BlockingCallMap m_blocking_calls;
	//! protection for m_blocking_calls
	Mutex m_blocking_call_lock;
	//! whether we're in a shutdown condition
	bool m_shutdown = false;
	//! used for storing the original fuse signal handlers
	SignalHandlerMap m_signal_handlers;

	//! the active umask of the current process
	static mode_t m_umask;

	//! currently existing windows that are ignored by us
	WindowSet m_ignored_windows;

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

