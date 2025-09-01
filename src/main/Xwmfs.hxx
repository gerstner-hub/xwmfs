#pragma once

// C++
#include <atomic>
#include <map>
#include <set>

// cosmos
#include <cosmos/io/EventFile.hxx>
#include <cosmos/io/Pipe.hxx>
#include <cosmos/io/Poller.hxx>
#include <cosmos/proc/SigAction.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/thread/Mutex.hxx>
#include <cosmos/thread/PosixThread.hxx>

// Xwmfs
#include "fuse/RootEntry.hxx"
#include "main/Options.hxx"
#include "x11/WinManagerWindow.hxx"

namespace xpp {

class CreateEvent;
class DestroyEvent;

}

namespace xwmfs {

class Entry;
class DesktopsRootDir;
class SelectionDirEntry;
class WindowsRootDir;
class WinManagerDirEntry;

/// The main application class that provides XWMFS functionality.
/**
 * This is the connector class between the X11 and FUSE related XWMFS
 * parts. It keeps the window manager information on the one hand and the
 * FUSE file system representation on the other hand.
 *
 * This is a singleton class such that it can be accessed globally from
 * anywhere. Any application-global data is kept here.
 *
 * This class also runs its own thread that is responsible for dealing
 * with events dispatched from Xlib to us. This allows us to update the
 * file system structure whenever relevant window manager information
 * changes.
 **/
class Xwmfs {
public: // functions

	/// Initialization routine that needs to be called from main() early on.
	static void early_init();

	/// Initialization routine that needs to be called by the FUSE initialization logic.
	/**
	 *  This function is only called from within FUSE initialization and
	 *  thus must not throw any exceptions.
	 *
	 * \return
	 *	EXIT_SUCCESS if initialization went well, else otherwise.
	 **/
	cosmos::ExitStatus init() noexcept;

	/// This function is called by FUSE for cleanup.
	void exit() noexcept;

	~Xwmfs();

	/// Returns the global XWMFS instance.
	static xwmfs::Xwmfs& getInstance() {
		static Xwmfs w;

		return w;
	}

	/// Lock for the X11 event handling in the event handling thread.
	/**
	 * This is to work around issues in libX11 itself. It's surrounding
	 * the multithreading, for example when calling XInternAtom from other
	 * threads then neither the separate Event thread nor the thread
	 * calling XInternAtom will unblock as long as no additional events
	 * are occurring that the event thread can consume.
	 *
	 * This is a FIXME in libX11 source file xcb_io.c in function
	 * _XReply() we're running into here. This lock here is supposed
	 * to prevent entering this situation by only calling into
	 * XNextEvent() when no other threads are actually doing
	 * asynchronous X calls.
	 *
	 * This currently mostly happens in the "properties" node where
	 * custom atom names are handled in the context of a FUSE thread.
	 **/
	cosmos::Mutex& getEventLock() { return m_event_lock; }

	/// Returns the global window manager window representation.
	WinManagerWindow& getRootWin() { return m_root_win; }

	/// Returns the application-wide XDisplay instance.
	auto& getDisplay() { return m_display; }

	/// Returns the file system structure root entry.
	RootEntry& getFS() { return m_fs_root; }

	xpp::XWindow& getSelectionWindow() { return m_selection_window; }

	/// Returns the options in effect for Xwmfs.
	xwmfs::Options& getOptions() { return m_opts; }

	/// Returns the umask of the current process.
	static cosmos::FileMode getUmask() { return m_umask; }

	/// Returns the current time (updated for each new X event)
	const cosmos::RealTime& getCurrentTime() const { return m_current_time; }

	/// Returns the "desktops" directory node.
	DesktopsRootDir* getDesktopsDir() { return m_desktop_dir; }

	/// Registers a blocking call situation.
	/**
	 * The calling thread will be associated with the given file object.
	 * The callee will make sure that fuse signals to abort the blocking
	 * request will be caught and the blocking call woken up in that
	 * situation.
	 *
	 * In any case the caller must call unregisterBlockingCall() after the
	 * blocking call is over, whether aborted or not.
	 *
	 * \return If `false` is returned then no blocking call can take place
	 * right now and the operation should be aborted with an error.
	 **/
	bool registerBlockingCall(Entry *f);

	/// Unregisters a previously registered blocking call situation.
	void unregisterBlockingCall();

protected: // functions

	friend void fuse_abort_signal(const cosmos::Signal);

	/// Called from an asynchronous signal handler in case a blocking shall be aborted.
	void abortBlockingCall(const bool all);

	/// Abort a blocking call for the given thread
	/**
	 * This is called synchronously from the event handling thread.
	 **/
	void abortBlockingCall(const cosmos::pthread::ID thread);

	/// Abort all pending blocking calls.
	void abortAllBlockingCalls();

	/// Read an available message from the abort pipe.
	void readAbortPipe();

	/// Sets up asynchronous signal handlers needed for aborting blocking calls.
	void setupAbortSignals(const bool on_off);

	/// Callback for Xlib protocol errors.
	/**
	 * If we don't overwrite this handler then the default handler from
	 * Xlib will call exit() in situations that can happen in normal
	 * operation to us like race conditions e.g. a window property updated
	 * but then the window also is destroyed but we still attempt to get
	 * the properties from the already dead window.
	 *
	 * For being able to recover such situations, we overwrite the error
	 * handler and simply print out information to stdout without exiting.
	 **/
	static int XErrorHandler(Display *disp, XErrorEvent *error);

	/// Callback for Xlib fatal error conditions.
	/**
	 * This is called if fatal errors occur like the X connection being
	 * lost. It's supposed not to return, otherwise Xlib calls exit().
	 **/
	static int XIOErrorHandler(Display *disp);

	/// Working loop for the event handling thread
	void eventThread();

	/// Processes all events that can currently be read without blocking.
	void handlePendingEvents();

	/// Handles a single X11 event received by the event thread.
	void handleEvent(const xpp::Event &ev);

	/// Handles a window creation event.
	/**
	 * \return
	 * 	Whether the window has been added to the file system or not
	 **/
	bool handleCreateEvent(const xpp::CreateEvent &ev);

	/// Handles a window destruction event
	void handleDestroyEvent(const xpp::DestroyEvent &ev);

	/// Handles any selection buffer related events.
	void handleSelectionEvent(const xpp::Event &ev);

	/// Returns whether the given event refers to pseudo window.
	bool isPseudoWindow(const xpp::CreateEvent &ev) const;

	bool isIgnored(const xpp::WinID win) const {
		return m_ignored_windows.find(win) != m_ignored_windows.end();
	}

private: // types

	using BlockingCallMap = std::map<cosmos::pthread::ID, Entry*>;
	using SignalHandlerMap = std::map<cosmos::Signal, cosmos::SigAction>;

	/// Different abort signal contexts
	enum class AbortType {
		/// Abort just a single ongoing call in the associated thread.
		CALL,
		/// Abort all ongoing blocking calls to prepare for shutdown.
		SHUTDOWN
	};

	struct AbortMsg {
		AbortType type;
		pthread_t thread = 0;
	};

	using WindowSet = std::set<xpp::WinID>;

private: // data

	/// The display we're operating on.
	xpp::XDisplay &m_display;

	/// Access to window manager information for the current X display (X11 part of XWMFS).
	WinManagerWindow m_root_win;

	/// The file system root entry that composes the complete file system (FUSE part of XWMFS).
	xwmfs::RootEntry m_fs_root;

	/// X11 event handling thread.
	cosmos::PosixThread m_ev_thread;
	/// Whether m_ev_thread should still be running.
	std::atomic_bool m_running;

	/// Options in effect for the current instance
	xwmfs::Options &m_opts;

	/// Wakeup signaling file descriptor.
	cosmos::EventFile m_wakeup_event;

	/// File descriptor poller for the event thread.
	cosmos::Poller m_event_poller;

	/// the time of the last event that might lead to creating new file system objects.
	cosmos::RealTime m_current_time;

	/// Directory node containing all windows.
	WindowsRootDir *m_win_dir = nullptr;
	/// Directory node containing all desktops and their windows.
	DesktopsRootDir *m_desktop_dir = nullptr;
	/// Directory node containing global window manager information.
	WinManagerDirEntry *m_wm_dir = nullptr;
	/// Directory node containing selection buffer information.
	SelectionDirEntry *m_selection_dir = nullptr;

	/// Abort pipe to signal abort requests for a specific thread.
	cosmos::Pipe m_abort_pipe;
	/// A mapping of active blocking threads and their associated files.
	BlockingCallMap m_blocking_calls;
	/// Synchronization for access to m_blocking_calls.
	cosmos::Mutex m_blocking_call_lock;
	/// Whether we're in a shutdown condition.
	bool m_shutdown = false;
	/// used for storing the original fuse signal handlers.
	SignalHandlerMap m_signal_handlers;

	/// The active umask of the current process.
	static cosmos::FileMode m_umask;

	/// Currently existing windows that are ignored by us.
	WindowSet m_ignored_windows;

	cosmos::Mutex m_event_lock;

	/// An XWindow created by us for managing selection buffers.
	xpp::XWindow m_selection_window;

private: // functions

	/// Private constructor to enforce singleton pattern.
	Xwmfs();

	/// Deleted copy constructor to enforce singleton pattern.
	Xwmfs(const Xwmfs &w) = delete;

	/// Creates the initial file system.
	void createFS();

	void createSelectionWindow();

	/// Print application state for debugging purposes.
	void printWMInfo();

	/// Updates m_current_time with the current time :-)
	void updateTime();
};

} // end ns
