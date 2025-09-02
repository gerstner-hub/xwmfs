// libxpp
#include <xpp/formatting.hxx> // needs to be the top include to prevent
			      // operator<< lookup errors
#include <xpp/AtomMapper.hxx>
#include <xpp/atoms.hxx>

// cosmos
#include <cosmos/string.hxx>
#include <cosmos/utils.hxx>

// xwmfs
#include "main/logger.hxx"
#include "main/Xwmfs.hxx"
#include "x11/WinManagerWindow.hxx"

namespace {

template <typename TYPE>
bool update_property(
		const xpp::XWindow &win,
		const xpp::AtomID atom, TYPE &property,
		xpp::Property<TYPE> *caller_prop = nullptr) {
	try {
		if (caller_prop) {
			/* for the need of this, see fetchDesktopNames() */
			win.getProperty(atom, *caller_prop);
			property = caller_prop->get();
		} else {
			xpp::Property<TYPE> tmp;
			win.getProperty(atom, tmp);
			property = tmp.get();
		}

		xwmfs::logger->debug() << "Property update acquired for "
			<< cosmos::to_integral(atom) << ": " << property << "\n";
		return true;
	} catch (const std::exception &ex) {
		xwmfs::logger->warn() << "Couldn't update property "
			<< cosmos::to_integral(atom)
			<< ": " << ex.what()  << std::endl;
		return false;
	}
}

template <typename TYPE>
void update_property(
		const xpp::XWindow &win,
		const xpp::AtomID atom,
		std::optional<TYPE> &property) {
	property = TYPE{};
	if (!update_property(win, atom, *property)) {
		property.reset();
	}
}

} // end anon ns

namespace xwmfs {

WinManagerWindow::WinManagerWindow(xpp::XDisplay &display) :
		xpp::RootWin{display} {
	logger->debug() << "root window has id: " << *this << std::endl;
	this->getInfo();
}

void WinManagerWindow::getInfo() {
	queryWMWindow();
	queryBasicWMProperties();
}

void WinManagerWindow::queryWMWindow() {
	/*
	 * This part is about checking for the presence of a compatible window
	 * manager. Both variants work the same but have different properties
	 * to check for.
	 *
	 * _WIN_SUPPORTING_WM_CHECK seems to be according to some pretty
	 * deprecated gnome WM specifications. This is expected to be an
	 * ordinal property. But fluxbox for example sets this to be a Window
	 * property.
	 *
	 * _NET_SUPPORTING_WM_CHECK seems to be an extension by EWMH and is a
	 * window property.
	 *
	 * In both cases, if the property is present in the root window then
	 * its value is the identifier for a valid child window created by the
	 * window manager.
	 *
	 * In the case of _NET_SUPPORTING_WM_CHECK the child window must also
	 * define the _NET_WM_NAME property which should contain the name of
	 * the window manager. In the other case the child only contains the
	 * same property as in the root window again with the same value.
	 *
	 * This seems to be the only way to portably and safely determine a
	 * compatible window manager is running.
	 */

	try {
		xpp::Property<xpp::WinID> child_window_prop;

		this->getProperty(xpp::atoms::ewmh_support_check,
			child_window_prop
		);

		m_ewmh_child = xpp::XWindow{child_window_prop.get()};

		logger->debug()
			<< "Child window of EWMH is: "
			<< m_ewmh_child << "\n";

		/*
		 * m_ewmh_child also needs to have the ewmh_support_check
		 * property set to the same ID (of itself). Otherwise this may
		 * be a stale window and the WM isn't actually running any
		 * more.
		 *
		 * EWMH says the application SHOULD check that. We're a nice
		 * application an do that.
		 */
		m_ewmh_child.getProperty(
			xpp::atoms::ewmh_support_check,
			child_window_prop
		);

		xpp::XWindow child2 = xpp::XWindow{child_window_prop.get()};

		if (m_ewmh_child == child2) {
			logger->debug() << "EWMH compatible WM is running!\n";
		} else {
			throw QueryError{"Couldn't reassure EWMH compatible WM running: IDs of child window and root window don't match"};
		}
	} catch(const std::exception &ex) {
		logger->error()
			<< "Couldn't query EWMH child window: " << ex.what()
			<< "\nSorry, can't continue without EWMH compatible WM running\n";
		throw;
	}
}

WinManagerWindow::WindowManager
WinManagerWindow::detectWM(const std::string &name) {
	const auto lower = cosmos::to_lower(name);

	// see whether this is a window manager known to us
	if (lower == "fluxbox") {
		return WindowManager::FLUXBOX;
	} else if(lower == "i3") {
		return WindowManager::I3;
	} else {
		return WindowManager::UNKNOWN;
	}
}

void WinManagerWindow::queryPID() {
	/*
	 * The PID of the process running the window, if existing MUST contain
	 * the PID and not something else.
	 * Spec doesn't say anything about whether the "window manager window"
	 * should set this and if it is set, whether it needs to be the PID of
	 * the WM.
	 */
	try {
		xpp::Property<int> wm_pid;

		m_ewmh_child.getProperty(xpp::atoms::ewmh_wm_pid, wm_pid);

		m_wm_pid = wm_pid.get();

		logger->debug() << "wm_pid acquired: " << *m_wm_pid << "\n";
		return;
	} catch (const std::exception &ex) {
		logger->warn() << "Couldn't query ewmh wm pid: " << ex.what() << "\n";
	}

	// maybe there's an alternative property for our specific WM
	std::string alt_pid_atom;

	switch (m_wm_type) {
	case WindowManager::FLUXBOX:
		alt_pid_atom = "_BLACKBOX_PID";
		break;
	case WindowManager::I3:
		alt_pid_atom = "I3_PID";
		break;
	default:
		break;
	}

	if (!alt_pid_atom.empty()) {
		// this WM hides its PID somewhere else
		try {
			xpp::Property<int> wm_pid;

			const xpp::AtomID atom_wm_pid(
				xpp::atom_mapper.mapAtom(alt_pid_atom)
			);
			this->getProperty(atom_wm_pid, wm_pid);

			m_wm_pid = wm_pid.get();
		} catch(const std::exception &ex) {
			logger->warn()
				<< "Couldn't query proprietary wm pid \""
				<< alt_pid_atom << "\": " << ex.what() << "\n";
		}
	}
}

void WinManagerWindow::queryBasicWMProperties() {
	/*
	 * The window manager name MUST be present according to EWMH spec. It
	 * is supposed to be in UTF8_STRING format. MUST only accounts for the
	 * window manager name. For client windows it may be present.
	 */
	try {
		m_wm_name = m_ewmh_child.getName();

		logger->debug() << "wm_name acquired: " << m_wm_name << "\n";

		m_wm_type = detectWM(m_wm_name);
	} catch (const std::exception &ex) {
		logger->warn() << "Couldn't query wm name: " << ex.what() << "\n";
	}

	queryPID();

	/*
	 *  The WM_CLASS should identify application class and name. This is
	 *  not EWMH specific but from ICCCM. Nothing's mentioned whether the
	 *  WM window needs to have this property.
	 */
	try {
		m_ewmh_child.getProperty(xpp::atoms::icccm_wm_class,
				m_wm_class);

		logger->debug() << "wm_class acquired: " << m_wm_class.get().str << "\n";
	} catch (const std::exception &ex) {
		logger->warn() << "Couldn't query wm class: " << ex.what() << "\n";
	}

	fetchShowDesktopMode();
	fetchNumDesktops();
	fetchDesktopNames();
	fetchActiveDesktop();
	fetchActiveWindow();
}

void WinManagerWindow::fetchNumDesktops() {
	update_property(
		*this,
		xpp::atoms::ewmh_wm_nr_desktops,
		m_wm_num_desktops
	);
}

void WinManagerWindow::setNumDesktops(const int num) {
	if (!hasNumDesktops()) {
		throw NotImplemented{};
	}

	this->sendRequest(xpp::atoms::ewmh_wm_nr_desktops, num);
}

void WinManagerWindow::setActiveDesktop(const int num) {
	if (!hasActiveDesktop()) {
		throw NotImplemented{};
	}

	this->sendRequest(xpp::atoms::ewmh_wm_cur_desktop, num);
}

void WinManagerWindow::fetchActiveDesktop() {
	update_property(
		*this,
		xpp::atoms::ewmh_wm_cur_desktop,
		m_wm_active_desktop
	);
}

void WinManagerWindow::fetchShowDesktopMode() {
	/*
	 * The _NET_WM_SHOWING_DESKTOP, if supported, indicates whether
	 * currently the "show the desktop" mode is active.
	 *
	 * This is EWMH specific. It's supposed to be an integer value of zero
	 * or one (thus a boolean value). The property is to be found on the
	 * root window, not the m_ewmh_child window.
	 */
	update_property(
		*this,
		xpp::atoms::ewmh_wm_desktop_shown,
		m_wm_showing_desktop
	);
}

void WinManagerWindow::setActiveWindow(const XWindow &win) {
	if (!hasActiveWindow()) {
		throw NotImplemented{};
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
	 * parameters and current state. For example it may set
	 * _NET_WM_STATE_DEMANDS_ATTENTION instead.
	 */
	this->sendRequest(
		xpp::atoms::ewmh_wm_active_window,
		(const char*)data,
		sizeof(data),
		&win
	);
}

void WinManagerWindow::moveToDesktop(const xpp::XWindow &win,
		const int desktop) {
	/*
	 * simply setting the property on the window does nothing. We need to
	 * send a request to the root window ...
	 *
	 * if the WM honors the request then it will set the property itself
	 * and we will get an update on the window.
	 */
	sendRequest(
		xpp::atoms::ewmh_window_desktop,
		desktop,
		&win
	);

}

void WinManagerWindow::fetchActiveWindow() {
	update_property(
		*this,
		xpp::atoms::ewmh_wm_active_window,
		m_wm_active_window
	);
}

void WinManagerWindow::fetchDesktopNames() {
	/*
	 * this is a pretty ugly area, libxpp's utf8_string structure is only
	 * a flat copy pointing into the xpp::Property data. After
	 * xpp::Property is released, the utf8_string pointers are no longer
	 * valid.
	 *
	 * Thus we are relying on a hack here, where the xpp::Property is hold
	 * locally here and pased into update_property() to keep the memory
	 * valid.
	 */
	std::vector<xpp::utf8_string> tmp_desktop_names;
	xpp::Property<std::vector<xpp::utf8_string>> prop_vector;

	m_wm_desktop_names.clear();

	update_property(*this, xpp::atoms::ewmh_wm_desktop_names,
			tmp_desktop_names,
			&prop_vector);

	for (auto utf8str: tmp_desktop_names) {
		m_wm_desktop_names.push_back(std::string{utf8str.str});
	}
}

} // end ns
