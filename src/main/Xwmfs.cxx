// C++
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <functional>
#include <vector>
#include <algorithm>

// C
#include <stdlib.h> // EXIT_*
#include <stdio.h>

// POSIX
#include <unistd.h> // pipe
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

// FUSE
#include <fuse.h>

// xwmfs
#include "main/Xwmfs.hxx"
#include "main/Options.hxx"
#include "main/StdLogger.hxx"
#include "main/WindowFileEntry.hxx"
#include "main/WindowDirEntry.hxx"
#include "main/WinManagerDirEntry.hxx"
#include "main/SelectionDirEntry.hxx"
#include "main/WindowsRootDir.hxx"
#include "fuse/xwmfs_fuse.hxx"
#include "fuse/Entry.hxx"
#include "common/Helper.hxx"
#include "x11/XAtom.hxx"

namespace xwmfs
{

mode_t Xwmfs::m_umask = 0777;

void Xwmfs::updateTime()
{
	m_current_time = time(nullptr);
}

void Xwmfs::createFS()
{
	updateTime();

	m_fs_root.setModifyTime(m_current_time);
	m_fs_root.setStatusTime(m_current_time);

	// window manager (wm) directory that contains files for wm
	// information
	m_wm_dir = new xwmfs::WinManagerDirEntry(m_root_win);
	m_fs_root.addEntry( m_wm_dir );

	// now add a directory that contains each window
	m_win_dir = new WindowsRootDir();
	m_fs_root.addEntry( m_win_dir );

	m_selection_dir = new xwmfs::SelectionDirEntry();
	m_fs_root.addEntry( m_selection_dir );

	const std::vector<XWindow> *windows = nullptr;

	if( m_opts.handlePseudoWindows() )
	{
		/*
		 * if we want to display all pseudo windows then we can't rely
		 * on the client list the window manager provides, because
		 * this only contains actual application windows.
		 *
		 * instead we need to query the complete window tree. from
		 * there on we get events for all created windows, even pseudo
		 * ones.
		 *
		 * this is only a snapshot so there may be a race condition
		 * and we end up with a slightly wrong initial state of
		 * windows. not sure what to do against that.
		 */
		m_root_win.queryTree();
		windows = &(m_root_win.getWindowTree());
	}
	else
	{
		m_root_win.queryWindows();
		windows = &(m_root_win.getWindowList());
	}

	// add each window found in the list

	FileSysWriteGuard write_guard(m_fs_root);

	for( const auto &win: *windows )
	{
		m_win_dir->addWindow( win, true, win == m_root_win );
	}
}

void Xwmfs::exit()
{
	m_fs_root.clear();

	if( m_ev_thread.getState() == xwmfs::Thread::RUN )
	{
		m_ev_thread.requestExit();

		const int dummy_data = 1;
		// we need to wakeup the thread to signal it that stuff is
		// over now
		const ssize_t write_res =
			::write(m_wakeup_pipe[1], &dummy_data, 1);

		assert( write_res != -1 );

		// finally join the thread
		m_ev_thread.join();
	}
}

int Xwmfs::XErrorHandler(Display *disp, XErrorEvent *error)
{
	(void)disp;
	(void)error;

	char err_msg[512];

	XGetErrorText(disp, error->error_code, &err_msg[0], 512);

	StdLogger::getInstance().warn()
		<< "An async X error occured: \"" << err_msg << "\"" << std::endl;

	return 0;
}

int Xwmfs::XIOErrorHandler(Display *disp)
{
	(void)disp;

	StdLogger::getInstance().error()
		<< "A fatal async X error occured. Exiting." << std::endl;

	// call the internal exit explicitly, the normal exit would cause
	// follow up errors through destruction of static objects in
	// unexpected states
	::_exit(1);
}

void Xwmfs::early_init()
{
	// to get the current umask we need to temporarily change the umask.
	// There seems to be no better system call. Starting from Linux 4.7 an
	// entry will be in /proc/<pid>/status, so we could read it from there
	// if present
	m_umask = ::umask(0777);
	(void)::umask(m_umask);

	// this asks the Xlib to be thread-safe
	// be careful that this must be the first Xlib call in the
	// process otherwise it won't work!
	if( ! ::XInitThreads() )
	{
		xwmfs_throw(
			xwmfs::Exception("Error initialiizing X11 threads")
		);
	}

	XPropTraits<utf8_string>::init();
}

int Xwmfs::init()
{
	int res = EXIT_SUCCESS;

	try
	{
		// sets the asynchronous error handler
		::XSetErrorHandler( & Xwmfs::XErrorHandler );
		::XSetIOErrorHandler( & Xwmfs::XIOErrorHandler );

		try
		{
			if( m_opts.xsync() )
			{
				::XSynchronize( XDisplay::getInstance(),
					true);

				xwmfs::StdLogger::getInstance().info()
					<< "Operating in Xlib synchronous mode\n";
			}

			// this gets us information about newly created
			// windows
			m_root_win.selectCreateEvent();
			// this gets us information about changed global
			// properties like the number of desktops
			m_root_win.selectPropertyNotifyEvent();
			// make sure the XServer knows about our event
			// registrations to avoid any race conditions
			XDisplay::getInstance().sync();

			/*
			 * there is a race condition that we can't really
			 * avoid here:
			 *
			 * in createFS() we statically determine the current
			 * state of the window manager. We might already be
			 * getting events about things that happened before
			 * this initial lookup, thus artifacts like
			 * destroy events for windows we don't know about or
			 * creation events for windows we've already seen.
			 *
			 * this is probably better than the other way around:
			 * losing events for things we should have known,
			 * causing inconsistent data
			 */

			createFS();

			m_ev_thread.start();

			setupAbortSignals(true);
		}
		catch( const xwmfs::RootWin::QueryError &ex )
		{
			xwmfs::Exception main_error(
				"Error querying window manager properties."
			);

			main_error.addError(ex);
			xwmfs_throw(main_error);
		}
	}
	catch( const xwmfs::Exception &ex )
	{
		res = EXIT_FAILURE;
		xwmfs::StdLogger::getInstance().error()
			<< "Error in FS operation: " << ex.what() << "\n";
	}
	catch( const std::exception &ex )
	{
		res = EXIT_FAILURE;
		xwmfs::StdLogger::getInstance().error()
			<< "Error in FS operation: " << ex.what() << "\n";
	}
	catch( ... )
	{
		res = EXIT_FAILURE;
		xwmfs::StdLogger::getInstance().error()
			<< "Error in FS operation: Unknown exception caught."
			" Terminating.\n";
	}

	return res;
}

Xwmfs::Xwmfs() :
	m_ev_thread(*this, "x11_event_thread"),
	m_opts( xwmfs::Options::getInstance() )
{
	// to get X events in a blocking way but still be able to react to
	// e.  g. a shutdown request we need to get the lower level file
	// descriptor that X is operating on.
	m_dis_fd = ::XConnectionNumber( XDisplay::getInstance() );

	// this is a pipe that allows us to wake up the event handling thread
	// in case of shutdown
	if( ::pipe2(m_wakeup_pipe, O_CLOEXEC) != 0 )
	{
		xwmfs_throw( SystemException("Unable to create wakeup pipe") );
	}

	// this is a pipe that allows to pass thread IDs to abort blocking
	// calls for to the event handling thread
	if( ::pipe2(m_abort_pipe, O_CLOEXEC) != 0 )
	{
		xwmfs_throw( SystemException("Unable to create abort pipe") );
	}

}

Xwmfs::~Xwmfs()
{
	FD_ZERO(&m_select_set);
	m_wm_dir = nullptr;
	m_win_dir = nullptr;

	::close( m_wakeup_pipe[0] );
	::close( m_wakeup_pipe[1] );

	::close( m_abort_pipe[0] );
	::close( m_abort_pipe[1] );
}

void Xwmfs::threadEntry(const xwmfs::Thread &t)
{
	m_display = XDisplay::getInstance();
	auto &logger = xwmfs::StdLogger::getInstance();

	const std::vector<int> fds(
		{ m_dis_fd, m_wakeup_pipe[0], m_abort_pipe[0] }
	);
	const int max_fd = *(std::max_element( fds.begin(), fds.end() )) + 1;

	while( t.getState() == xwmfs::Thread::RUN )
	{
		FD_ZERO(&m_select_set);
		for( int fd: fds )
		{
			FD_SET(fd, &m_select_set);
		}

		// here we wait until one of the file descriptors is readable
		const int sel_res = ::select(
			max_fd,
			&m_select_set, nullptr, nullptr, nullptr
		);

		if( sel_res == -1 )
		{
			::perror("unable to select on event fds");
			return;
		}

		// if the pipe is readable then we have to shutdown
		if( FD_ISSET(m_wakeup_pipe[0], &m_select_set) )
		{
			logger.info()
				<< "Caught cancel request. Shutting down..."
				<< std::endl;
			return;
		}
		else if( FD_ISSET(m_abort_pipe[0], &m_select_set) )
		{
			readAbortPipe();
			continue;
		}

		// now we should be able to read at least one event without
		// blocking
		handlePendingEvents();
	}
}

void Xwmfs::handlePendingEvents()
{
	MutexGuard g(m_event_lock);

	/*
	 * this is important to avoid blocking while there are still events to
	 * be processed. this is because the select() in the event thread only
	 * wakes up when there's network data from the X server. But the
	 * libX11 can read more than one event at once from the network in one
	 * go, thus the socket might not be readable, still there would be
	 * pending events that we wouldn't process
	 */
	while( XPending(m_display) != 0 )
	{
		XNextEvent(m_display, &m_ev);

		try
		{
			// don't keep this lock for the duration of the
			// handling, because otherwise we might run into
			// cross-locking issues, because other threads may
			// hold the FS lock and want our event lock, while he
			// have the event lock but desire the FS lock.
			MutexReverseGuard rg(m_event_lock);
			handleEvent(m_ev);
		}
		catch(const xwmfs::Exception &ex)
		{
			auto &logger = xwmfs::StdLogger::getInstance();
			logger.error()
				<< "Failed to handle X11 event of type "
				<< std::dec << m_ev.type << ": " << ex.what();
		}
	}
}

void Xwmfs::handleEvent(const XEvent &ev)
{
	auto &logger = xwmfs::StdLogger::getInstance();
#if 0
	logger.debug() << "Received event #" << ev.xany.serial << " of type "
		<< std::dec << ev.type << std::endl;
#endif

	switch( ev.type )
	{
	// a new window came into existence
	case CreateNotify:
	{
		if( ! handleCreateEvent(ev.xcreatewindow) )
		{
			m_ignored_windows.insert( ev.xcreatewindow.window );
		}
		break;
	}
	// a window was destroyed
	case DestroyNotify:
	{
		handleDestroyEvent(ev.xdestroywindow);
		auto it = m_ignored_windows.find( ev.xdestroywindow.window );
		if( it != m_ignored_windows.end() )
		{
			// don't need to process a window we've ignored
			// before
			m_ignored_windows.erase(it);
			break;
		}
		break;
	}
	case PropertyNotify:
	{
		logger.debug()
			<< "Property (" << XAtom(ev.xproperty.atom) << ")"
			<< " on window " << ev.xproperty.window << " changed ("
			<< std::dec << ev.xproperty.state << ")" << std::endl;

		const bool is_delete = ev.xproperty.state == PropertyDelete;

		switch( ev.xproperty.state )
		{
		case PropertyDelete:
		case PropertyNewValue:
		{
			XWindow w(ev.xproperty.window);
			updateTime();

			FileSysWriteGuard write_guard(m_fs_root);

			if( w == m_root_win )
			{
				is_delete ?
					m_wm_dir->delProp(ev.xproperty.atom) :
					m_wm_dir->update(ev.xproperty.atom);
			}
			else
			{
				is_delete ?
					m_win_dir->deleteProperty(w, ev.xproperty.atom) :
					m_win_dir->updateProperty(w, ev.xproperty.atom);
			}
			break;
		}
		default:
			break;
		}
	}
	// called upon window size/appearance changes
	case ConfigureNotify:
	{
		XWindow w(ev.xconfigure.window);
		updateTime();

		FileSysWriteGuard write_guard(m_fs_root);

		m_win_dir->updateGeometry(w, ev.xconfigure);
		break;
	}
	case CirculateNotify:
	{
		break;
	}
	// called if parts of the window become
	// visible/invisible
	case UnmapNotify:
	case MapNotify:
	{
		XWindow w(ev.xmap.window);
		if( isIgnored(w) )
		{
			break;
		}
		updateTime();
		FileSysWriteGuard write_guard(m_fs_root);
		m_win_dir->updateMappedState( w, ev.type == MapNotify );
		break;
	}
	case GravityNotify:
	{
		break;
	}
	case ReparentNotify:
	{
		XWindow w(ev.xreparent.window);
		w.setParent(ev.xreparent.parent);
		FileSysWriteGuard write_guard(m_fs_root);
		m_win_dir->updateParent(w);
		break;
	}
	default:
		logger.debug()
			<< __FUNCTION__
			<< ": Some unknown event "
			<< ev.type << " for window "
			<< XWindow(ev.xany.window) << " received" << "\n";
		break;
	}
}

bool Xwmfs::isPseudoWindow(const XCreateWindowEvent &ev) const
{
	auto &debug_log = xwmfs::StdLogger::getInstance().debug();

	// Xlib manual says one should generally ignore these
	// events as they come from popups
	if( ev.override_redirect )
	{
		debug_log << "Ignoring override_redirect window " << ev.window << std::endl;
		return true;
	}
	// this is grand-kid or something. we could add these
	// in a hierarchical manner as sub-windows but for now
	// we ignore them
	else if( ev.parent != m_root_win.id() )
	{
		debug_log << "Ignoring grand-child-window" << ev.window << std::endl;
		return true;
	}

	return false;
}

bool Xwmfs::handleCreateEvent(const XCreateWindowEvent &ev)
{
	if( ! m_opts.handlePseudoWindows() )
	{
		if( isPseudoWindow(ev) )
		{
			return false;
		}
	}

	auto &debug_log = xwmfs::StdLogger::getInstance().debug();

	XWindow w(ev.window);
	w.setParent(ev.parent);

	debug_log << "Window " << w << " was created!" << std::endl;
	debug_log << "\tParent: " << XWindow(w.getParent()) << std::endl;
	debug_log << "\twin name = ";

	try
	{
		debug_log << w.getName() << std::endl;
	}
	catch( const xwmfs::Exception &ex )
	{
		debug_log << "error getting win name: " << ex << std::endl;
	}

	try
	{
		updateTime();
		FileSysWriteGuard write_guard(m_fs_root);
		m_win_dir->addWindow(w);
		m_wm_dir->windowLifecycleEvent(w, true);
	}
	catch( const xwmfs::Exception &ex )
	{
		debug_log << "\terror adding window: " << ex << std::endl;
	}


	return true;
}

void Xwmfs::handleDestroyEvent(const XDestroyWindowEvent &ev)
{
	auto &debug_log = xwmfs::StdLogger::getInstance().debug();
	XWindow w(ev.window);

	debug_log << "Window " << w << " was destroyed!" << std::endl;

	FileSysWriteGuard write_guard(m_fs_root);
	m_win_dir->removeWindow(w);
	m_wm_dir->windowLifecycleEvent(w, false);
}

/*
 * global sync signal handler for the fuse abort signal
 */
void fuseAbortSignal(int sig)
{
	auto &xwmfs = Xwmfs::getInstance();

	const bool shutdown = sig != SIGUSR1;

	xwmfs.abortBlockingCall(shutdown);
}

void Xwmfs::setupAbortSignals(const bool on_off)
{
	/*
	 * we have two troubles with implementing blocking read calls in the
	 * EventFile here:
	 *
	 * 1) when a blocking call is pending and the userspace process that
	 * blocks on it wants to interrupt the call then this situation is
	 * only made available to us via SIGUSR1.  This signal will be sent by
	 * fuse in a thread directed manner i.e. we need to setup an
	 * asynchronous signal handler and the thread this signal handlers
	 * runs in is the one that needs to abort its blocking request.
	 *
	 * Our EventFile implementation uses a Condition variable for
	 * efficiently waiting for data, and the Condition variable has no way
	 * to unblock due to an asynchronously received signal (contrary to
	 * what man 7 signal says about EINTR return if SA_RESTART is not
	 * set).
	 *
	 * Thus we need to somehow keep track of which blocking calls are
	 * going on in which objects by which threads. Then the asynchronous
	 * signal handler forwards the interrupt information to a global event
	 * thread which sorts the information out and unblocks the right
	 * thread. Pretty fucked up but that's just the way it is
	 *
	 * https://sourceforge.net/p/fuse/mailman/message/28816288/
	 *
	 * 2) When a blocking call is pending and the fuse userspace process
	 * gets a SIGINT or SIGTERM for shutdown then fuse will internally
	 * deadlock.
	 *
	 * When FUSE gets the interrupt it tries to join all of its running
	 * threads before calling the fuse_exit() function. So we have no
	 * chance of knowing that we'd need to unblock the Condition wait.
	 *
	 * The way fuse thinks we should deal with this is using
	 * pthread_cancel and cancelation points. but this doesn't fly with us
	 * in c++ where we need to call destructors and friends.
	 *
	 * Therefore we're overwriting the SIGINT and SIGTERM signal handlers
	 * so we get a chance to abort all ongoing blocking calls. The tricky
	 * part is to forward the interrupt request to the internal fuse
	 * routines, however.
	 *
	 * a) For this we need to know the struct fuse which is a low level
	 * data structure normally not available when we use the high level
	 * API of fuse. We need this structure for explicitly shutting down
	 * fuse when we intercept a SIGINT or SIGTERM.
	 *
	 * b) Another alternative is to catch the SIGINT, unblock our blocking
	 * threads and then reinstate the original SIGINT handler for fuse to
	 * shut down the regular way.
	 *
	 * Both approaches leave room for a race condition when we've
	 * unblocked our waiting threads but before fuse has a chance to
	 * shutdown another blocking call comes into existence.  There's
	 * nothing much we can do against this, except maybe polling for
	 * blocked threads or so. Or a shutdown flag which we now have in
	 * m_shutdown.
	 *
	 * Actually the fuse_get_context() is only valid for the duraction of
	 * a request call, but I hope the fuse pointer is an exception, as
	 * this shouldn't change for the lifetime of the fuse instance.
	 *
	 * After some testing I'm going for solution b). This works now.
	 * Solution a) had strange problems when the fuse main thread didn't
	 * react to the fuse_exit() call on the first attempt. It seems the
	 * sem_wait() the fuse main thread does did not always return EINTR in
	 * these situations. All pretty f***ed up.
	 */
	struct sigaction act, orig;
	std::memset( &act, 0, sizeof(struct sigaction) );
	act.sa_handler = &fuseAbortSignal;

	std::vector<int> sigs;

	sigs.push_back(SIGUSR1);
	sigs.push_back(SIGINT);
	sigs.push_back(SIGTERM);

	for( const auto sig: sigs )
	{
		if( sigaction(sig, on_off ? &act : &m_signal_handlers[sig], &orig) != 0  )
		{
			xwmfs_throw(xwmfs::Exception("Failed to change abort sighandler"));
		}

		if( on_off )
		{
			m_signal_handlers[sig] = orig;
		}
	}
}

void Xwmfs::abortBlockingCall(const bool all)
{
	// send the ID of the thread that got the signal over the abort pipe
	// so the event thread can deal with the situation without
	// restrictions of async signal handling

	AbortMsg msg;
	msg.type = all ? AbortType::SHUTDOWN : AbortType::CALL;
	msg.thread = pthread_self();

	// writing to the pipe <= PIPE_BUF is atomic so we don't need to fear
	// partial writes here. But the call may block if the pipe is full.
	if( write(m_abort_pipe[1], &msg, sizeof(msg)) != sizeof(msg) )
	{
		std::cerr << "failed to write to abort pipe\n";
	}
}

void Xwmfs::abortBlockingCall(pthread_t thread)
{
	auto &logger = xwmfs::StdLogger::getInstance();
	MutexGuard g(m_blocking_call_lock);

	auto it = m_blocking_calls.find(thread);

	if( it == m_blocking_calls.end() )
	{
		logger.error() << "Failed to find abort entry for thread" << std::endl;
		return;
	}

	logger.info() << "Abort request for some blocking call" << std::endl;

	auto ef = it->second;

	ef->abortBlockingCall(thread);
}

void Xwmfs::abortAllBlockingCalls()
{
	MutexGuard g(m_blocking_call_lock);

	for( auto it: m_blocking_calls )
	{
		auto ef = it.second;

		ef->abortBlockingCall(it.first);
	}

	/*
	 * this signaling flag is necessary to avoid race conditions: all
	 * blocking calls may return but userspace programs often react to
	 * EINTR by retrying the blocking calls, which would cause again a
	 * deadlock. This flag helps us to continously return EINTR to
	 * successive calls.
	 */
	m_shutdown = true;
}

void Xwmfs::readAbortPipe()
{
	AbortMsg msg;

	// read <= PIPE_BUF bytes from the pipe is atomic,
	// should never even block
	if( read(m_abort_pipe[0], &msg, sizeof(msg)) != sizeof(msg) )
	{
		auto &logger = xwmfs::StdLogger::getInstance();
		logger.error()
			<< "Failed to read from abort pipe"
			<< std::endl;
		return;
	}

	if( msg.type == AbortType::CALL )
	{
		abortBlockingCall(msg.thread);
	}
	else
	{
		abortAllBlockingCalls();
		/* reinstate original signal handlers */
		setupAbortSignals(false);
		kill( getpid(), SIGINT );
	}
}

bool Xwmfs::registerBlockingCall(Entry *f)
{
	MutexGuard g(m_blocking_call_lock);

	if( m_shutdown )
	{
		return false;
	}

	m_blocking_calls.insert( std::make_pair( pthread_self(), f ) );

	return true;
}

void Xwmfs::unregisterBlockingCall()
{
	MutexGuard g(m_blocking_call_lock);

	m_blocking_calls.erase( pthread_self() );
}

} // end ns

