// own header
#include "x11/XWindow.hxx"
#include "x11/XAtom.hxx"

#include "main/Xwmfs.hxx"

namespace xwmfs
{

XWindow::XWindow(Window win) :
	m_win(win),
	m_std_props(StandardProps::instance())
{
}
	
std::string XWindow::getName() const
{
	try
	{
		xwmfs::Property<xwmfs::utf8_string> utf8_name;

		this->getProperty(m_std_props.atom_ewmh_window_name, utf8_name);

		return utf8_name.get().data;
	}
	catch( ... )
	{ }

	/*
	 *	If EWMH name property is not present then try to fall back to
	 *	ICCCM WM_NAME property
	 *	(at least I think that is ICCCM). This will not be in UTF8 but
	 *	in XA_STRING format
	 */

	xwmfs::Property<const char*> name;

	this->getProperty(m_std_props.atom_icccm_window_name, name);

	return name.get();
}

pid_t XWindow::getPID() const
{
	xwmfs::Property<int> pid;

	this->getProperty(m_std_props.atom_ewmh_window_pid, pid);

	return pid.get();
}

int XWindow::getDesktop() const
{
	xwmfs::Property<int> desktop_nr;

	this->getProperty(m_std_props.atom_ewmh_window_desktop, desktop_nr);

	return desktop_nr.get();
}


void XWindow::setName(const std::string &name)
{
	// XXX should try ewmh_name first
	
	xwmfs::Property<const char*> name_prop( name.c_str() );

	this->setProperty( m_std_props.atom_icccm_window_name, name_prop );
}

void XWindow::setDesktop(const int num)
{
	/*
	 * simply setting the property does nothing. We need to send a request
	 * to the root window ...
	 *
	 * if the WM honors the request then it will set the property itself
	 * and we will get an update
	 */
	xwmfs::Xwmfs::getInstance().getRootWin().sendRequest(
		m_std_props.atom_ewmh_window_desktop,
		num,
		this
	);
}

void XWindow::destroy()
{
	auto &display = XDisplay::getInstance();
	const auto res = XDestroyWindow( display, m_win );
	display.flush();

	// this is one of these asynchronous error reportings that's not
	// helpful. See setProperty().
	if( false /* || res != Success*/ )
	{
		xwmfs_throw( X11Exception(display, res) );
	}
}

void XWindow::sendDeleteRequest()
{
	long data[2];
	data[0] = m_std_props.atom_icccm_wm_delete_window;
	data[1] = CurrentTime;

	sendRequest(
		m_std_props.atom_icccm_wm_protocols,
		(const char*)&data[0],
		sizeof(data),
		this
	);
}

void XWindow::sendRequest(
	const XAtom &message,
	const char *data,
	const size_t len,
	const XWindow *window
)
{
	auto &logger = xwmfs::StdLogger::getInstance();

	logger.debug()
		<< "Sending request to window " << *this << ":"
		<< "msg = " << message << " with " << len << " bytes of data, window = "
		<< (window ? window->id() : 0) << std::endl;
	XEvent event;
	std::memset( &event, 0, sizeof(event) );

	if( len > sizeof(event.xclient.data) )
	{
		xwmfs_throw( Exception("XEvent data exceeds maximum") );
	}

	auto &display = XDisplay::getInstance();

	event.xclient.type = ClientMessage;
	event.xclient.serial = 0;
	event.xclient.send_event = True;
	event.xclient.message_type = message;
	event.xclient.window = window ? window->id() : 0;
	event.xclient.format = 32;
	std::memcpy(event.xclient.data.b, data, len);

	Status s = XSendEvent(
		display,
		this->id(),
		False,
		m_send_event_mask,
		&event
	);

	if( s == BadValue || s == BadWindow )
	{
		xwmfs_throw(X11Exception(XDisplay::getInstance(), s));
	}

	// make sure the event gets sent out
	display.flush();
}

void XWindow::selectEvent(const long new_event) const
{
	m_input_event_mask |= new_event;

	const int res = ::XSelectInput(
		XDisplay::getInstance(), m_win, m_input_event_mask
	);

	// stupid return codes again
	(void)res;

}

} // end ns

