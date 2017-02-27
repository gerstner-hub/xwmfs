#include "x11/RootWin.hxx"
#include "main/StdLogger.hxx"
#include "common/Helper.hxx"

namespace xwmfs
{

RootWin::RootWin() :
	XWindow(
		DefaultRootWindow(
			static_cast<Display*>(XDisplay::getInstance())
		)
	),
	m_wm_name(),
	m_wm_class(),
	m_windows()
{
	xwmfs::StdLogger::getInstance().debug()
		<< "root window has id: " 
		<< std::hex << std::showbase << this->id() << std::dec
		<< std::endl;
	this->getInfo();

	// the event mask influences which X clients will receive the event.
	// For the root window to react to our requests these masks seem to be
	// helpful. 0 value doesn't work. I'm not sure if this is not too
	// broad but there is no clear documentation which specific value
	// might be correct for our use cases
	m_send_event_mask = SubstructureRedirectMask | SubstructureNotifyMask;
}

void RootWin::getInfo()
{
	queryWMWindow();
	queryBasicWMProperties();
	queryWindows();
}

void RootWin::queryWMWindow()
{
	auto &logger = xwmfs::StdLogger::getInstance();

	/*
	 *	This part is about checking for the presence of a compatible
	 *	window manager. Both variants work the same but have different
	 *	properties to check for.
	 *
	 *	_WIN_SUPPORTING_WM_CHECK seems to be according to some pretty
	 *	deprecated gnome WM specifications. This is expected to be an
	 *	ordinal property. But fluxbox for example sets this to be a
	 *	Window property.
	 *
	 *	_NET_SUPPORTING_WM_CHECK seems to be an extension by EWMH and
	 *	is a window property.
	 *
	 *	In both cases, if the property is present in the root window
	 *	then its value is the identifier for a valid child window
	 *	created by the window manager.
	 *
	 *	In the case of _NET_SUPPORTING_WM_CHECK the child window must
	 *	also define the _NET_WM_NAME property which should contain the
	 *	name of the window manager. In the other case the child only
	 *	contains the same property as in the root window again with
	 *	the same value.
	 *
	 *	This seems to be the only way to portably and safely determine
	 *	a compatible window manager is running.
	 */

	try
	{
		Property<Window> child_window_prop;

		this->getProperty(
			m_std_props.atom_ewmh_support_check,
			child_window_prop
		);

		m_ewmh_child = XWindow( child_window_prop.get() );

		logger.debug() << "Child window of EWMH is: "
			<< std::hex << std::showbase << m_ewmh_child.id()
			<< std::dec << "\n";

		/*
		 *	m_ewmh_child also needs to have
		 *	m_support_property_ewmh set to the same ID (of
		 *	itself). Otherwise this may be a stale window and the
		 *	WM isn't actually running any more.
		 *
		 *	EWMH says the application SHOULD check that. We're a
		 *	nice application an do that.
		 */
		m_ewmh_child.getProperty(
			m_std_props.atom_ewmh_support_check,
			child_window_prop
		);

		xwmfs::XWindow child2 = XWindow( child_window_prop.get() );

		if( m_ewmh_child == child2 )
		{
			logger.debug() << "EWMH compatible WM is running!\n";
		}
		else
		{
			xwmfs_throw(QueryError(
				"Couldn't reassure EWMH compatible WM running:"\
				"IDs of child window and root window don't match"
			));
		}
	}
	catch( const xwmfs::Exception &ex )
	{
		logger.error()
			<< "Couldn't query EWMH child window: " << ex.what()
			<< "\nSorry, can't continue without EWMH compatible"\
			" WM running\n";
		throw;
	}
}

RootWin::WindowManager RootWin::detectWM(const std::string &name)
{
	const auto lower = tolower(name);

	// see whether this is a window manager known to us
	if( lower == "fluxbox" )
	{
		return WindowManager::FLUXBOX;
	}
	else if( lower == "i3" )
	{
		return WindowManager::I3;
	}
	else
	{
		return WindowManager::UNKNOWN;
	}
}

void RootWin::queryPID()
{
	/*
	 *	The PID of the process running the window, if existing MUST
	 *	contain the PID and not something else.
	 *	Spec doesn't say anything about whether the "window manager
	 *	window" should set this and if it is set whether it needs to
	 *	be the PID of the WM.
	 */
	try
	{
		Property<int> wm_pid;

		m_ewmh_child.getProperty(m_std_props.atom_ewmh_wm_pid, wm_pid);

		m_wm_pid = wm_pid.get();

		xwmfs::StdLogger::getInstance().debug() <<
			"wm_pid acquired: " << m_wm_pid << "\n";

		return;
	}
	catch( const xwmfs::Exception &ex )
	{
		xwmfs::StdLogger::getInstance().warn()
			<< "Couldn't query ewmh wm pid: " << ex.what();
	}

	// maybe there's an alternative property for our specific WM
	std::string alt_pid_atom;

	switch( m_wm_type )
	{
	case WindowManager::FLUXBOX:
		alt_pid_atom = "_BLACKBOX_PID";
		break;
	case WindowManager::I3:
		alt_pid_atom = "I3_PID";
		break;
	default:
		break;
	}

	if( ! alt_pid_atom.empty() )
	{
		// this WM hides its PID somewhere else
		try
		{
			Property<int> wm_pid;

			const XAtom atom_wm_pid(
				XAtomMapper::getInstance().getAtom(alt_pid_atom)
			);
			this->getProperty(atom_wm_pid, wm_pid);

			m_wm_pid = wm_pid.get();
		}
		catch( const xwmfs::Exception &ex )
		{
			xwmfs::StdLogger::getInstance().warn()
				<< "Couldn't query proprietary wm pid \""
				<< alt_pid_atom << "\": " << ex.what();
		}
	}
}	

void RootWin::queryBasicWMProperties()
{
	/*
	 *	The window manager name MUST be present according to EWMH
	 *	spec. It is supposed to be in UTF8_STRING format. MUST only
	 *	accounts for the window manager name. For client windows it
	 *	may be present.
	 */
	try
	{
		m_wm_name = m_ewmh_child.getName();

		xwmfs::StdLogger::getInstance().debug() <<
			"wm_name acquired: " << m_wm_name << "\n";

		m_wm_type = detectWM(m_wm_name);
	}
	catch( const xwmfs::Exception &ex )
	{
		xwmfs::StdLogger::getInstance().warn()
			<< "Couldn't query wm name: " << ex.what();
	}

	queryPID();

	/*
	 * 	The WM_CLASS should identify application class and name. This
	 * 	is not EWMH specific but from ICCCM. Nothing's mentioned
	 * 	whether the WM window needs to have this property.
	 */
	try
	{
		m_ewmh_child.getProperty(XA_WM_CLASS, m_wm_class);

		xwmfs::StdLogger::getInstance().debug()
			<< "wm_class acquired: "
			<< m_wm_class.get().data << "\n";
	}
	catch( const xwmfs::Exception &ex )
	{
		xwmfs::StdLogger::getInstance().warn()
			<< "Couldn't query wm class: " << ex.what();
	}

	updateShowingDesktop();
	updateNumberOfDesktops();
	updateActiveDesktop();
	updateActiveWindow();
}


void RootWin::updateShowingDesktop()
{
	/*
	 * 	The _NET_WM_SHOWING_DESKTOP, if supported, indicates whether
	 * 	currently the "show the desktop" mode is active.
	 *
	 * 	This is EWMH specific. It's supposed to be an integer value of
	 * 	zero or one (thus a boolean value). The property is to be
	 * 	found on the root window, not the m_ewmh_child window.
	 */
	updateProperty(
		m_std_props.atom_ewmh_wm_desktop_shown,
		m_wm_showing_desktop
	);
}

void RootWin::updateActiveDesktop()
{
	updateProperty(
		m_std_props.atom_ewmh_wm_cur_desktop,
		m_wm_active_desktop
	);
}

void RootWin::updateActiveWindow()
{
	updateProperty(
		m_std_props.atom_ewmh_wm_active_window,
		m_wm_active_window
	);
}

void RootWin::updateNumberOfDesktops()
{
	updateProperty(
		m_std_props.atom_ewmh_wm_nr_desktops,
		m_wm_num_desktops
	);
}


template <typename TYPE>
void RootWin::updateProperty(const XAtom &atom, TYPE &property)
{
	auto &logger = xwmfs::StdLogger::getInstance();

	try
	{
		Property<TYPE> tmp;

		this->getProperty(atom, tmp);

		property = tmp.get();

		logger.debug() << "Property update acquired for "
			<< atom << ": " << property << "\n";
	}
	catch( const xwmfs::Exception &ex )
	{
		logger.warn() << "Couldn't update property " << atom
			<< ": " << ex.what()  << std::endl;
	}
}

void RootWin::queryWindows()
{
	/*
	 * 	The _NET_CLIENT_LIST, if supported, is set on the root window
	 * 	and contains an array of X windows that are managed by the WM.
	 *
	 * 	According to EWMH the array is in "initial mapping order"
	 * 	which I guess is the order in which windows have been created
	 * 	(or more correct: mapped). Alongside there is the
	 * 	_NET_CLIENT_LIST_STACKING property that contains the same data
	 * 	but in "bottom-to-top stacked order" which means it is ordered
	 * 	according to the layer the window is in.
	 *
	 * 	NOTE: There's also _WIN_CLIENT_LIST with a similar purpose,
	 * 	probably from ICCCM
	 */
	try
	{
		Property<std::vector<Window> > windows;

		this->getProperty(m_std_props.atom_ewmh_wm_window_list, windows);

		const std::vector<Window> &wins = windows.get();

		xwmfs::StdLogger::getInstance().debug()
			<< "window list acquired:\n";

		for( const auto &win: wins )
		{
			m_windows.push_back( xwmfs::XWindow( win ) );
			xwmfs::StdLogger::getInstance().debug()
				<< "- " << win << "\n";
		}

	}
	catch( const xwmfs::Exception &ex )
	{
		xwmfs::StdLogger::getInstance().warn()
			<< "Couldn't query window list: " << ex.what();
		throw;
	}

}

void RootWin::setWM_ActiveDesktop(const int &num)
{
	if( ! hasWM_ActiveDesktop() )
	{
		xwmfs_throw(NotImplemented());
	}

	this->sendRequest(m_std_props.atom_ewmh_wm_cur_desktop, num);
}

void RootWin::setWM_ActiveWindow(const XWindow &win)
{
	if( ! hasWM_ActiveWindow() )
	{
		xwmfs_throw(NotImplemented());
	}

	long data[3];
	// source indication. 0 == outdated client, 1 == application, 2 ==
	// pager. Suppose we're a pager.
	data[0] = 2;
	// timestamp of the last user activity in the client. Since we have no
	// real window associated we can set this to zero. Could also set it
	// to the current time if we count the write request as an activity
	data[1] = 0;
	// our currently active window, which I again guess is zero for "we
	// have none"
	data[2] = 0;
	/*
	 * the window manager may ignore this request based on our provided
	 * parameters and current states. For example it may set
	 * _NET_WM_STATE_DEMANDS_ATTENTION instead.
	 */
	this->sendRequest(
		m_std_props.atom_ewmh_wm_active_window,
		(const char*)data,
		sizeof(data),
		&win
	);
}

void RootWin::setWM_NumDesktops(const int &num)
{
	if( ! hasWM_NumDesktops() )
	{
		xwmfs_throw(NotImplemented());
	}

	this->sendRequest(m_std_props.atom_ewmh_wm_nr_desktops, num);
}

} // end ns

