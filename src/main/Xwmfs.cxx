// C++
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <functional>
#include <vector>

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
#include "fuse/xwmfs_fuse.hxx"
#include "fuse/EventFile.hxx"
#include "common/Helper.hxx"

namespace xwmfs
{

mode_t Xwmfs::m_umask = 0777;

/*
 *	Upon a destroy event for a window this function is called for the
 *	according window.
 *
 *	It is possible that the given window isn't existing in the filesystem
 *	in which case the event should be ignored.
 *
 *	Exceptions are handled by the caller
 */
void Xwmfs::removeWindow(const XWindow &win)
{
	FileSysWriteGuard write_guard(m_fs_root);

	std::stringstream id;
	id << win.id();

	m_win_dir->removeEntry(id.str());
}

void Xwmfs::updateTime()
{
	m_current_time = time(nullptr);
}

/*
 *	Upon create event for a window this function is called for the
 *	according window
 */
void Xwmfs::addWindow(const XWindow &win)
{
	// get the current time for timestamping the new file
	updateTime();
	FileSysWriteGuard write_guard(m_fs_root);

	// we want to get any structure change events
	win.selectDestroyEvent();
	win.selectPropertyNotifyEvent();

	// the window directories are named after their IDs
	m_win_dir->addEntry(new xwmfs::WindowDirEntry(win), false);

	auto &logger = xwmfs::StdLogger::getInstance().debug();
	logger << "New window " << win.id() << std::endl;
}

void Xwmfs::updateRootWindow(Atom changed_atom)
{
	FileSysWriteGuard write_guard(m_fs_root);

	m_wm_dir->update(changed_atom);
}

void Xwmfs::updateWindow(const XWindow &win, Atom changed_atom)
{
	FileSysWriteGuard write_guard(m_fs_root);

	std::stringstream id;
	id << win.id();

	// TODO: this is an unsafe cast, because we have no type information
	// for WindowDirEntry ... :-/
	WindowDirEntry* win_dir = reinterpret_cast<WindowDirEntry*>(
		m_win_dir->getDirEntry(id.str().c_str())
	);

	if( !win_dir )
	{
		xwmfs::StdLogger::getInstance().debug() <<
			"Property update for unknown window" << std::endl;
		return;
	}

	win_dir->update(changed_atom);
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
	m_win_dir = new xwmfs::DirEntry("windows");
	m_fs_root.addEntry( m_win_dir );

	// add each window there
	const auto &windows = m_root_win.getWindowList();

	for( const auto &win: windows )
	{
		addWindow( win );
	}
}

void Xwmfs::exit()
{
	m_fs_root.clear();

	if( m_ev_thread.getState() == xwmfs::Thread::RUN )
	{
		const int dummy_data = 1;
		// we need to wakeup the thread to signal it that stuff is
		// over now
		const ssize_t write_res =
			::write(m_wakeup_pipe[1], &dummy_data, 1);

		assert( write_res != -1 );

		// finally join the thread
		m_ev_thread.join();
	}

	setupAbortSignals(false);
}

int Xwmfs::XErrorHandler(Display *disp, XErrorEvent *error)
{
	(void)disp;
	(void)error;

	char err_msg[512];

	XGetErrorText(disp, error->error_code, &err_msg[0], 512);

	StdLogger::getInstance().warn()
		<< "An X error occured: \"" << err_msg << "\"" << std::endl;

	return 0;
}

int Xwmfs::XIOErrorHandler(Display *disp)
{
	(void)disp;

	StdLogger::getInstance().error()
		<< "A fatal X error occured. Exiting." << std::endl;

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
	XEvent ev;
	Display *dis = XDisplay::getInstance();

	auto &logger = xwmfs::StdLogger::getInstance();

	while( t.getState() == xwmfs::Thread::RUN )
	{
		FD_ZERO(&m_select_set);
		FD_SET(m_dis_fd, &m_select_set);
		FD_SET(m_wakeup_pipe[0], &m_select_set);
		FD_SET(m_abort_pipe[0], &m_select_set);

		// here we wait until one of the file descriptors is readable
		const int sel_res = ::select(
			FD_SETSIZE,
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

		// now we should be able to read the next event without
		// blocking
		XNextEvent(dis, &ev);

		try
		{
			handleEvent(ev);
		}
		catch(const xwmfs::Exception &ex)
		{
			logger.error()
				<< "Failed to handle X11 event of type "
				<< std::dec << ev.type << ": " << ex.what();
		}
	}
}

void Xwmfs::handleEvent(const XEvent &ev)
{
	auto &logger = xwmfs::StdLogger::getInstance();
#if 0
	logger.debug() << "Received event of type "
		<< std::dec << ev.type << std::endl;
#endif

	switch( ev.type )
	{
	// a new window came into existence
	//
	// NOTE: it might be better to look for MappedEvents instead
	// of CreateEvents. In case there are strange hidden windows
	// and such
	case CreateNotify:
	{
		handleCreateEvent(ev);
		break;
	}
	// a window was destroyed
	case DestroyNotify:
	{
		XWindow w(ev.xdestroywindow.window);
		logger.debug() << "Window " << w << " was destroyed!" << std::endl;
		this->removeWindow(w);
		break;
	}
	case PropertyNotify:
	{
		logger.debug() << "Property (atom = "
			<< std::dec << ev.xproperty.atom
			<< ") on window " << std::hex
			<< "0x" << ev.xproperty.window << " changed ("
			<< std::dec << ev.xproperty.state << ")" << std::endl;
		switch( ev.xproperty.state )
		{
		case PropertyNewValue:
		{
			XWindow w(ev.xproperty.window);
			updateTime();

			if( w == m_root_win )
			{
				this->updateRootWindow(ev.xproperty.atom);
			}
			else
			{
				this->updateWindow(w, ev.xproperty.atom);
			}
			break;
		}
		case PropertyDelete:
		default:
			break;
		}
	}
	// called upon window size/appearance changes
	case ConfigureNotify:
	{
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
		break;
	}
	case GravityNotify:
	{
		break;
	}
	case ReparentNotify:
	{
		break;
	}
	default:
		logger.debug()
			<< __FUNCTION__
			<< ": Some unknown event received" << "\n";
		break;
	}
}

void Xwmfs::handleCreateEvent(const XEvent &ev)
{
	// Xlib manual says one should generally ignore these
	// events as they come from popups
	if( ev.xcreatewindow.override_redirect )
		return;
	// this is grand-kid or something. we could add these
	// in a hierarchical manner as sub-windows but for now
	// we ignore them
	else if( ev.xcreatewindow.parent != m_root_win.id() )
		return;

	XWindow w(ev.xcreatewindow.window);

	auto &debug_log = xwmfs::StdLogger::getInstance().debug();

	debug_log << "Window " << w << " was created!" << std::endl;
	debug_log << "\tParent: " << XWindow(ev.xcreatewindow.parent) << std::endl;
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
		this->addWindow(w);
	}
	catch( const xwmfs::Exception &ex )
	{
		debug_log << "\terror adding window: " << ex << std::endl;
	}
}

/*
 * global sync signal handler for the fuse abort signal
 */
void fuseAbortSignal(int sig)
{
	auto &xwmfs = Xwmfs::getInstance();

	xwmfs.abortBlockingCall(sig != SIGUSR1);
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
	 * For this we need to know the struct fuse which is a low level data
	 * structure normally not available when we use the high level API of
	 * fuse. We need this structure for explicitly shutting down fuse when
	 * we intercept a SIGINT or SIGTERM.
	 *
	 * Another alternative might have been to catch the SIGINT, unblock
	 * our blocking threads and then reinstate the original SIGINT handler
	 * for fuse to shut down the regular way.
	 *
	 * Both approaches leave room for a race condition when we've
	 * unblocked our waiting threads but before fuse has a chance to
	 * shutdown another blocking call comes into existence.  There's
	 * nothing much we can do against this, except maybe polling for
	 * blocked threads or so.
	 *
	 * Actually the fuse_get_context() is only valid for the duraction of
	 * a request call, but I hope the fuse pointer is an exception, as
	 * this shouldn't change for the lifetime of the fuse instance.
	 */
	if( on_off )
	{
		m_fuse = fuse_get_context()->fuse;
	}

	struct sigaction act;
	std::memset( &act, 0, sizeof(struct sigaction));
	act.sa_handler = on_off ? &fuseAbortSignal : SIG_DFL;

	std::vector<int> sigs;

	sigs.push_back(SIGUSR1);

	if( on_off )
	{
		// those signals should never be reset to default, because
		// then the process would end unconditionally on successive
		// SIGINT/SIGTERM
		sigs.push_back(SIGINT);
		sigs.push_back(SIGTERM);
	}

	for( const auto sig: sigs )
	{
		if( sigaction(sig, &act, nullptr) != 0  )
		{
			xwmfs_throw(xwmfs::Exception("Failed to change abort sighandler"));
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
		// forward the shutdown request to fuse, this is a low level
		// API we're actually not suppose to use, along with the
		// illegally acquired m_fuse. Cross fingers ;)
		fuse_exit(m_fuse);
	}
}

void Xwmfs::registerBlockingCall(EventFile *f)
{
	MutexGuard g(m_blocking_call_lock);

	m_blocking_calls.insert( std::make_pair( pthread_self(), f ) );
}

void Xwmfs::unregisterBlockingCall()
{
	MutexGuard g(m_blocking_call_lock);

	m_blocking_calls.erase( pthread_self() );
}

} // end ns

