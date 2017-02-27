// C++
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <functional>

// C
#include <stdlib.h> // EXIT_*
#include <stdio.h>

// POSIX
#include <unistd.h> // pipe
#include <sys/select.h>
#include <sys/stat.h>

// xwmfs
#include "main/Xwmfs.hxx"
#include "main/Options.hxx"
#include "main/StdLogger.hxx"
#include "main/WindowFileEntry.hxx"
#include "main/WindowDirEntry.hxx"
#include "main/WinManagerDirEntry.hxx"
#include "fuse/xwmfs_fuse.hxx"
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
	const int pipe_res = ::pipe(m_wakeup_pipe);

	if( pipe_res != 0 )
	{
		xwmfs_throw( SystemException("Unable to create wakeup pipe") );
	}
}

Xwmfs::~Xwmfs()
{
	FD_ZERO(&m_select_set);
	m_wm_dir = nullptr;
	m_win_dir = nullptr;

	::close( m_wakeup_pipe[0] );
	::close( m_wakeup_pipe[1] );
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

		// here we wait until one of the file descriptors is readable
		const int sel_res = ::select(
			FD_SETSIZE,
			&m_select_set, nullptr, nullptr, nullptr
		);

		if( sel_res == -1 )
		{
			::perror("unable to select X-connection fd");
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
			<< "0x" << ev.xproperty.window << " changed"
			<< std::dec << std::endl;
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

} // end ns

