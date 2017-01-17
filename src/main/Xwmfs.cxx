#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <functional>

// EXIT_*
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h> // pipe
#include <sys/select.h>
#include <sys/stat.h>

#include "main/Xwmfs.hxx"
#include "fuse/xwmfs_fuse.hxx"
#include "main/Options.hxx"
#include "main/StdLogger.hxx"
#include "common/Helper.hxx"

namespace xwmfs
{

mode_t Xwmfs::m_umask = 0777;
			
/**
 * \brief
 * 	A FileEntry that is associated with an XWindow object
 * \details
 * 	This type is used for all files found within windows directories in
 * 	the file system. Depending on the actual file called the right
 * 	operations are performed at the associated window.
 **/
struct WindowFileEntry :
	public FileEntry
{
	//! Creates a WindowFileEntry associated with \c win
	WindowFileEntry(
		const std::string &n,
		const XWindow& win,
		const time_t &t = 0,
		const bool writable = true
	) :
		FileEntry(n, writable, t),
		m_win(win)
	{ }

	/**
	 * \brief
	 * 	Implementation of write() that updates window properties
	 * 	according to the file that is being written
	 **/
	int write(const char *data, const size_t bytes, off_t offset) override
	{
		// we don't support writing at offsets
		if( offset )
			return -EOPNOTSUPP;
		
		try
		{
			auto it = m_write_member_function_map.find(m_name);

			if( it == m_write_member_function_map.end() )
			{
				xwmfs::StdLogger::getInstance().error()
					<< __FUNCTION__
					<< ": Write call for window file"
					" entry of unknown type: \""
					<< this->m_name
					<< "\"\n";
				return -ENXIO;
			}

			auto mem_fn = it->second;

			(this->*(mem_fn))(data, bytes);
		}
		catch( const xwmfs::Exception &e )
		{
			xwmfs::StdLogger::getInstance().error()
				<< __FUNCTION__
				<< ": Error operating on window (node '"
				<< this->m_name << "'): "
				<< e.what() << std::endl;
			return -EINVAL;
		}
			
		return bytes;
	}

	void writeName(const char *data, const size_t bytes)
	{
		std::string name(data, bytes);
		m_win.setName( name );
	}

	void writeDesktop(const char *data, const size_t bytes)
	{
		int the_num;
		const auto parsed = parseInteger(
			data, bytes, the_num
		);

		if( parsed >= 0 )
		{
			m_win.setDesktop( the_num );
		}
	}

	void writeCommand(const char *data, const size_t bytes);

	/**
	 * \brief
	 * 	Compares this file system entries against the given window
	 * \details
	 * 	If this file system entry is associated with \c w then \c true
	 * 	is returned. \c false otherwise.
	 **/
	bool operator==(const XWindow &w) const { return m_win == w; }
	bool operator!=(const XWindow &w) const { return !((*this) == w); }

	/**
	 * \brief
	 * 	Casts the object to its associated XWindow type
	 **/
	operator XWindow&() { return m_win; }

protected: // types

	typedef void (WindowFileEntry::*WriteMemberFunction)(const char*, const size_t);
	typedef std::map<std::string, WriteMemberFunction> WriteMemberFunctionMap;

protected: // data

	//! XWindow associated with this FileEntry
	XWindow m_win; // XXX currently a flat copy

	// a mapping of file system names to their associated write functions
	static const WriteMemberFunctionMap m_write_member_function_map;
};

const WindowFileEntry::WriteMemberFunctionMap WindowFileEntry::m_write_member_function_map = {
	{ "name", &WindowFileEntry::writeName },
	{ "desktop", &WindowFileEntry::writeDesktop },
	{ "command", &WindowFileEntry::writeCommand }
};

void WindowFileEntry::writeCommand(const char *data, const size_t bytes)
{
	const auto command = tolower(stripped(std::string(data, bytes)));

	if( command == "destroy" )
	{
		m_win.destroy();
	}
	else if( command == "delete" )
	{
		m_win.sendDeleteRequest();
	}
	else
	{
		throw xwmfs::Exception(
			XWMFS_SRC_LOCATION, "invalid command encountered"
		);
	}
}

/**
 * \brief
 * 	A FileEntry that is associated with a global window manager entry
 * \details
 * 	This is a specialized FileEntry for particular global entries relating
 * 	to the window manager. Mostly this is only used for writable files to
 * 	relay the write request correctly.
 **/
struct WinManagerEntry : 
	public FileEntry
{
	WinManagerEntry(const std::string &n, const time_t &t = 0) :
		FileEntry(n, true, t)
	{}

	int write(const char *data, const size_t bytes, off_t offset) override
	{
		// we don't support writing at offsets
		if( offset )
			return -EOPNOTSUPP;

		auto root_win = xwmfs::Xwmfs::getInstance().getRootWin();
		auto &logger = xwmfs::StdLogger::getInstance();

		try
		{
			int the_num = 0;
			const auto parsed = parseInteger(data, bytes, the_num);

			if( this->m_name == "active_desktop" )
			{
				if( parsed >= 0 )
				{
					root_win.setWM_ActiveDesktop(the_num);
				}
			}
			else if( this->m_name == "number_of_desktops" )
			{
				if( parsed >= 0 )
				{
					root_win.setWM_NumDesktops(the_num);
				}
			}
			else if( this->m_name == "active_window" )
			{
				if( parsed >= 0 )
				{
					root_win.setWM_ActiveWindow(
						XWindow(the_num)
					);
				}
			}
			else
			{
				logger.warn()
					<< __FUNCTION__
					<< ": Write call for win manager file of unknown type: \""
					<< this->m_name
					<< "\"\n";
				return -ENXIO;
			}
		}
		catch( const xwmfs::XWindow::NotImplemented &e )
		{
			return -ENOSYS;
		}
		catch( const xwmfs::Exception &e )
		{
			logger.error()
				<< __FUNCTION__ << ": Error setting window manager property ("
				<< this->m_name << "): " << e.what() << std::endl;
			return -EINVAL;
		}


		// claim we wrote all content. This "eats up" things like
		// newlines passed with 'echo'.  Otherwise programs may try to
		// write the "missing" bytes which turns into an offset write
		// than which we don't support
		return bytes;
	}
};

/*
 *	Upon destroy event for a window this function is called for the
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
 *
 *	XXX create class for filesystem that has member functions add/remove
 *	for reuse of code
 */
void Xwmfs::addWindow(const XWindow &win)
{
	// get the current time for timestamping the new file
	updateTime();
	FileSysWriteGuard write_guard(m_fs_root);

	std::stringstream id;
	id << win.id();

	// we want to get any structure change events
	win.selectDestroyEvent();
	win.selectPropertyNotifyEvent();

	// the window directories are named after their IDs
	DirEntry *win_dir = m_win_dir->addEntry(
		new xwmfs::DirEntry(id.str(), m_current_time), false );

	// add an ID file (when symlinks point there then it's easier this way
	// to find out the ID of the window
	FileEntry *win_id = win_dir->addEntry(
		new xwmfs::FileEntry("id", false, m_current_time),
		false
	);

	// the content for the ID file is the ID, of course
	*win_id << id.rdbuf() << '\n';
		
	xwmfs::StdLogger::getInstance().debug()
		<< "New window " << win.id() << std::endl;

	addWindowName(*win_dir, win);
	addDesktopNumber(*win_dir, win);
	addPID(*win_dir, win);
	addCommandControl(*win_dir, win);
}

void Xwmfs::addWindowName(DirEntry &win_dir, const XWindow &win)
{
	try
	{
		std::string name_str = win.getName();
		FileEntry *win_name = win_dir.addEntry(
			new xwmfs::WindowFileEntry("name", win, m_current_time),
			false
		);
		*win_name << name_str << '\n';
	}
	catch( const xwmfs::Exception &ex )
	{
		// this can happen legally. It is a race condition. We've been
		// so fast to register the window but it hasn't got a name
		// yet.
		//
		// The name will be noticed later on via a property update.
		xwmfs::StdLogger::getInstance().debug()
			<< "Couldn't get name for window " << win.id()
			<< " right away" << std::endl;
	}
}
	
void Xwmfs::addDesktopNumber(DirEntry &win_dir, const XWindow &win)
{
	try
	{
		const int desktop_num = win.getDesktop();
		FileEntry *win_desktop = win_dir.addEntry(
			new xwmfs::WindowFileEntry("desktop", win, m_current_time),
			false
		);
		*win_desktop << desktop_num << '\n';
	}
	catch( const xwmfs::Exception &ex )
	{
		xwmfs::StdLogger::getInstance().debug()
			<< "Couldn't get desktop nr. for window right away"
			<< std::endl;
	}
}

void Xwmfs::addPID(DirEntry &win_dir, const XWindow &win)
{
	try
	{
		const int pid = win.getPID();
		FileEntry *win_pid = win_dir.addEntry(
			new xwmfs::WindowFileEntry("pid", win, m_current_time, false),
			false
		);
		*win_pid << pid << '\n';
	}
	catch( const xwmfs::Exception &ex )
	{
		xwmfs::StdLogger::getInstance().debug()
			<< "Couldn't get pid for window right away"
			<< std::endl;
	}
}

void Xwmfs::addCommandControl(DirEntry &win_dir, const XWindow &win)
{
	auto entry = win_dir.addEntry(
		new xwmfs::WindowFileEntry("command", win, m_current_time, true)
	);

	for( const auto command: { "destroy", "delete" } )
	{
		// provide the available commands as read content
		*entry << command << " ";
	}

	entry->seekp(-1, entry->cur);
	(*entry) << "\n";
}

void Xwmfs::updateRootWindow(Atom changed_atom)
{
	updateTime();
	FileSysWriteGuard write_guard(m_fs_root);
	
	auto &logger = xwmfs::StdLogger::getInstance();
	auto std_props = StandardProps::instance();

	FileEntry *entry = nullptr;
	
	if( std_props.atom_ewmh_wm_nr_desktops == changed_atom )
	{
		m_root_win.updateNumberOfDesktops();

		entry = (FileEntry*)m_wm_dir->getEntry("number_of_desktops");
		if( ! entry )
			// shouldn't happen
			return;

		entry->str("");

		try
		{
			*entry << m_root_win.getWM_NumDesktops() << "\n";
		}
		catch(...)
		{
			logger.error()
				<< "Error udpating number_of_desktops property"
				<< std::endl;
		}
	}
	else if( std_props.atom_ewmh_wm_cur_desktop == changed_atom )
	{
		m_root_win.updateActiveDesktop();

		entry = (FileEntry*)m_wm_dir->getEntry("active_desktop");
		if( ! entry )
			// shouldn't happen
			return;

		entry->str("");

		try
		{
			*entry << m_root_win.getWM_ActiveDesktop() << "\n";
		}
		catch(...)
		{
			logger.error()
				<< "Error udpating number_of_desktops property"
				<< std::endl;
		}

	}
	else if( std_props.atom_ewmh_wm_active_window == changed_atom )
	{
		m_root_win.updateActiveWindow();

		entry = (FileEntry*)m_wm_dir->getEntry("active_window");
		if( ! entry )
			// shouldn't happen
			return;

		entry->str("");

		try
		{
			*entry << m_root_win.getWM_ActiveWindow() << "\n";
		}
		catch(...)
		{
			logger.error()
				<< "Error updating active_window property"
				<< std::endl;
		}
	}
	else
	{
		logger.warn()
			<< "Root window unknown property "
			<< "0x" << changed_atom << ") changed" << std::endl;
	}

	if( entry )
	{
		entry->setModifyTime(m_current_time);
	}
}

void Xwmfs::updateWindow(const XWindow &win, Atom changed_atom)
{
	updateTime();
	FileSysWriteGuard write_guard(m_fs_root);

	std::stringstream id;
	id << win.id();

	DirEntry* win_dir = (DirEntry*)m_win_dir->getEntry(id.str());

	if( !win_dir )
	{
		xwmfs::StdLogger::getInstance().debug() <<
			"Property update for unknown window" << std::endl;
		return;
	}

	auto std_props = StandardProps::instance();

	/*
	 * TODO: make a table like
	 *
	 * atom_id -> (parent_dir, file, update_func)
	 *
	 * to make this code more compact
	 */

	WindowFileEntry *entry = nullptr;

	if( std_props.atom_icccm_window_name == changed_atom )
	{
		entry = (WindowFileEntry*)win_dir->getEntry("name");
		if( ! entry )
		{
			// the window name was not available during creation
			// but now here it is
			addWindowName(*win_dir, win);
			return;
		}

		entry->str("");

		try
		{
			*entry << win.getName() << "\n";
		}
		catch(...)
		{
			xwmfs::StdLogger::getInstance().error() <<
				"Error udpating name property" << std::endl;
		}
	}
	else if( std_props.atom_ewmh_desktop_nr == changed_atom )
	{
		try
		{
			const int desktop_num = win.getDesktop();

			xwmfs::StdLogger::getInstance().debug()
				<< "Got desktop nr. update: "
				<< desktop_num << std::endl;
		
			entry = (WindowFileEntry*)win_dir->getEntry("desktop");
			if( ! entry )
			{
				// the desktop number was not available during
				// creation but now here it is
				addDesktopNumber(*win_dir, win);
				return;
			}

			entry->str("");
			*entry << desktop_num << "\n";
		}
		catch( ... )
		{
			xwmfs::StdLogger::getInstance().error() <<
				"Error udpating desktop property" << std::endl;
		}
	}
			
	if( entry )
	{
		entry->setModifyTime(m_current_time);
	}
}

void Xwmfs::createFS()
{
	updateTime();

	m_fs_root.setModifyTime(m_current_time);
	m_fs_root.setStatusTime(m_current_time);

	// window manager (wm) directory that contains files for wm
	// information
	m_wm_dir = new xwmfs::DirEntry("wm");

	m_fs_root.addEntry( m_wm_dir );
		
	// create pid entry in wm dir
	FileEntry *wm_pid = m_wm_dir->addEntry( new xwmfs::FileEntry("pid") );

	if( m_root_win.hasWM_Pid() )
		*wm_pid << m_root_win.getWM_Pid() << '\n';
	else
		*wm_pid << "-1" << '\n';

	// add name entry in wm dir
	FileEntry *wm_name =
		m_wm_dir->addEntry( new xwmfs::FileEntry("name", false) );

	*wm_name << (m_root_win.hasWM_Name() ?
		m_root_win.getWM_Name() : "N/A") << '\n';

	// add "show desktop mode" entry in wm dir
	FileEntry *wm_sdm = m_wm_dir->addEntry(
		new xwmfs::FileEntry("show_desktop_mode") );
	if( m_root_win.hasWM_ShowDesktopMode() )
		*wm_sdm << m_root_win.getWM_ShowDesktopMode() << '\n';
	else
		*wm_sdm << "-1" << '\n';

	FileEntry *wm_nr_desktops = m_wm_dir->addEntry(
		new xwmfs::WinManagerEntry("number_of_desktops")
	);

	*wm_nr_desktops << m_root_win.getWM_NumDesktops() << '\n';

	// add wm class entry in wm dir
	FileEntry *wm_class = m_wm_dir->addEntry(
		new xwmfs::FileEntry("class") );
	*wm_class << (m_root_win.hasWM_Class() ?
		m_root_win.getWM_Class() : "N/A" ) << '\n';

	// add an entry reflecting the active desktop and also allowing to
	// change it
	FileEntry *wm_active_desktop = m_wm_dir->addEntry(
		new xwmfs::WinManagerEntry("active_desktop")
	);
	*wm_active_desktop << (m_root_win.hasWM_ActiveDesktop() ?
		m_root_win.getWM_ActiveDesktop() : -1 ) << '\n';

	FileEntry *wm_active_window = m_wm_dir->addEntry(
		new xwmfs::WinManagerEntry("active_window")
	);

	*wm_active_window << (m_root_win.hasWM_ActiveWindow() ?
		m_root_win.getWM_ActiveWindow() : Window(0) ) << '\n';

	// now add a directory that contains each window
	m_win_dir = new xwmfs::DirEntry("windows");
	m_fs_root.addEntry( m_win_dir );

	// add each window there
	const std::vector<xwmfs::XWindow> &windows = m_root_win.getWindowList();
	std::stringstream id;
	for(
		std::vector<xwmfs::XWindow>::const_iterator win =
			windows.begin();
		win != windows.end();
		win++ )
	{
		addWindow( *win );
	}

#if 0
	// test the findEntry function or RootEntry
	Entry * found_entry;

	const char* const DIRS[] = { "/", "/wm", "/wm/", "/wm/name", "/wm/name/" };
	
	for( size_t dir_index = 0; dir_index < 5; dir_index++ )
	{
		std::cout << "looking up " << DIRS[dir_index] << std::endl;
		found_entry = xwmfs::filesystem->findEntry(DIRS[dir_index]);

		if( found_entry )
		{
			std::cout << "Found it! It's a " <<
				(found_entry->type() == Entry::DIRECTORY ?
				"Directory" : "File") << std::endl;
		}
		else
		{
			std::cout << "Not found!" << std::endl;
		}
	}
#endif
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
	
int Xwmfs::XErrorHandler(
	Display *disp,
	XErrorEvent *error)
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
		xwmfs::Exception main_error(
			XWMFS_SRC_LOCATION,
			"Error initialiizing X11 threads"
		);
		throw main_error;
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
#if 0
			printWmInfo();
#endif
		}
		catch( const xwmfs::RootWin::QueryError &ex )
		{
			xwmfs::Exception main_error(
				XWMFS_SRC_LOCATION,
				"Error querying window manager properties.");

			main_error.addError(ex);
			throw main_error;
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

#if 0
void Xwmfs::printWMInfo()
{
	std::cout << "Global window manager information:\n\n";
	std::cout << "Name: " << (m_root_win.hasWM_Name() ? m_root_win.getWM_Name() : "N/A") << "\n";
	std::cout << "PID: ";
	if( m_root_win.hasWM_Pid() )
		std::cout << m_root_win.getWM_Pid();
	else
		std::cout << "N/A";
	std::cout << "\n";
	std::cout << "Class: " << (m_root_win.hasWM_Class() ? m_root_win.getWM_Class() : "N/A") << "\n";
	std::cout << "ShowingDesktop: ";
	if( m_root_win.hasWM_ShowDesktopMode() )
		std::cout << m_root_win.getWM_ShowDesktopMode();
	else
		std::cout << "N/A";

	std::cout << "\nWindow list:\n";
	const std::vector<xwmfs::XWindow> &windows = m_root_win.getWindowList();
	for( std::vector<xwmfs::XWindow>::const_iterator win = windows.begin(); win != windows.end(); win++ )
	{
		std::cout << "ID = " << win->id() << ", Name = \"" << win->getName() << "\"\n";
	}
	std::cout << "\n\n";
}
#endif

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
		// TODO: exception
		::perror("unable to create wakeup pipe");
		abort();
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
			// Xlib manual says one should generally ignore these
			// events as they come from popups
			if( ev.xcreatewindow.override_redirect )
				break;
			// this is grand-kid or something. we could add these
			// in a hierarchical manner as sub-windows but for now
			// we ignore them
			else if( ev.xcreatewindow.parent != m_root_win.id() )
				break;
				
			XWindow w(ev.xcreatewindow.window);
			
			auto &debug_log = logger.debug();

			debug_log
				<< "Window "
				<< w
				<< " was created!" << std::endl;

			debug_log
				<< "\tParent: "
				<< XWindow(ev.xcreatewindow.parent)
				<< std::endl;

			debug_log
				<< "\twin name = ";

			try
			{
				debug_log
					<< w.getName()
					<< std::endl;
			}
			catch( const xwmfs::Exception &ex )
			{
				debug_log
					<< "error getting win name: "
					<< ex
					<< std::endl;
			}

			try
			{
				this->addWindow(w);
			}
			catch( const xwmfs::Exception &ex )
			{
				debug_log
					<< "\terror adding window: "
					<< ex
					<< std::endl;
			}

			break;
		}
		// a window was destroyed
		case DestroyNotify:
		{
			XWindow w(ev.xdestroywindow.window);
			logger.debug()
				<< "Window " << w
				<< " was destroyed!"
				<< std::endl;
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
}

} // end ns

