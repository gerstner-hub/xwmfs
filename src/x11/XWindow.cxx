// C++
#include <sstream>
#include <iomanip>

// own header
#include "x11/XWindow.hxx"
#include "x11/XWindowAttrs.hxx"
#include "x11/XAtom.hxx"

#include "main/Xwmfs.hxx"

namespace xwmfs
{

XWindow::PropertyTypeMismatch::PropertyTypeMismatch(
	Atom expected, Atom encountered
) : Exception("Retrieved property has different type than expected: ")
{
	std::ostringstream s;
	s << "Expected " << expected << " but encountered " \
		<< encountered;
	m_error += s.str();
}

XWindow::PropertyChangeError::PropertyChangeError(
	Display *dis, const int errcode
) :
	X11Exception(dis, errcode)
{
	m_error += ". While trying to change property.";
}

XWindow::PropertyQueryError::PropertyQueryError(
	Display *dis, const int errcode
) :
	X11Exception(dis, errcode)
{
	m_error += ". While trying to get property.";
}

XWindow::XWindow(Window win) :
	m_win(win),
	m_std_props(StandardProps::instance())
{
}

std::string XWindow::idStr() const
{
	std::stringstream id;
	id << this->id();

	return id.str();
}

std::string XWindow::getName() const
{
	try
	{
		xwmfs::Property<utf8_string> utf8_name;

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
	try
	{
		xwmfs::Property<utf8_string> utf8_name;
		utf8_name = utf8_string(name.c_str());

		this->setProperty( m_std_props.atom_ewmh_window_name, utf8_name );

		return;
	}
	catch( ... )
	{
	}

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

std::string XWindow::getClientMachine() const
{
	xwmfs::Property<const char *> name;

	this->getProperty(m_std_props.atom_icccm_wm_client_machine, name);

	return name.get();
}

void XWindow::destroy()
{
	auto &display = XDisplay::getInstance();
	const auto res = XDestroyWindow( display, m_win );
	display.flush();

	if( res != 1 )
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

	const Status s = XSendEvent(
		display,
		this->id(),
		False,
		m_send_event_mask,
		&event
	);

	if( s == BadValue || s == BadWindow )
	{
		xwmfs_throw(X11Exception(display, s));
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
	if( res == 0 )
	{
		xwmfs_throw(Exception("XSelectInput failed"));
	}
}

template <typename PROPTYPE>
void XWindow::getProperty(const Atom name_atom, Property<PROPTYPE> &prop) const
{
	// shorthand for our concrete property object
	typedef Property<PROPTYPE> THIS_PROP;

	Atom x_type = THIS_PROP::getXType();
	assert( x_type != None );

	Atom actual_type;
	// if 8, 16, or 32-bit format was actually read
	int actual_format = 0;
	unsigned long ret_items = 0;
	unsigned long remaining_bytes = 0;
	unsigned char *data = nullptr;

	const int res = XGetWindowProperty(
		XDisplay::getInstance(),
		m_win,
		name_atom,
		// offset into the property data
		0,
		// maximum length of the property to read in 32-bit items
		4096 / 4,
		// delete request
		False,
		// our expected type
		x_type,
		// actually present type, format, number of items
		&actual_type,
		&actual_format,
		&ret_items,
		&remaining_bytes,
		// where data is stored
		&data
	);

	// note: on success data is memory allocated by Xlib. data always
	// contains one excess byte that is set to zero thus its possible to
	// use data as a c-string without copying it.
	if ( res != Success )
	{
		xwmfs_throw(PropertyQueryError(XDisplay::getInstance(), res));
	}

	if( actual_type == None )
	{
		XFree(data);
		xwmfs_throw(PropertyNotExisting());
	}
	else if( x_type != actual_type )
	{
		XFree(data);
		xwmfs_throw(PropertyTypeMismatch(x_type, actual_type));
	}

	assert( actual_format == THIS_PROP::Traits::format );
	assert( ! remaining_bytes );

	// ret_items gives the number of items acc. to actual_format that have
	// been returned
	prop.takeData(data, ret_items * (actual_format / 8));
}

template <typename PROPTYPE>
void XWindow::setProperty(const Atom name_atom, const Property<PROPTYPE> &prop)
{
	/*
	 * NOTE: currently only performs mode PropModeReplace
	 *
	 * I don't think that prepend or append are very common use cases.
	 */
	// shorthand for our concrete Property object
	typedef Property<PROPTYPE> THIS_PROP;

	Atom x_type = THIS_PROP::getXType();
	assert( x_type != None );

	const int siz = THIS_PROP::Traits::getNumElements( prop.get() );

	auto &display = XDisplay::getInstance();

	const int res = XChangeProperty(
		display,
		m_win,
		name_atom,
		x_type,
		THIS_PROP::Traits::format,
		PropModeReplace,
		(unsigned char*)prop.getRawData(),
		siz
	);

	// XChangeProperty returns constantly 1 which would result in
	// "BadRequest".
	// Actual errors are dispatched asynchronously via the functions set
	// at XSetErrorHandler. Not very helpful as hard to process
	// asynchronously and inefficient to synchronize to the asynchronous
	// result.
	(void)res;

	// requests to the server are not dispatched immediatly thus we need
	// to flush once
	display.flush();
}

void XWindow::getAttrs(XWindowAttrs &attrs)
{
	auto &display = XDisplay::getInstance();
	const auto status = XGetWindowAttributes(display, m_win, &attrs);

	// stupid error codes again. A non-zero status on success?
	if( status == 0 )
	{
		xwmfs_throw(X11Exception(display, status));
	}
}

void XWindow::updateFamily()
{
	auto &display = XDisplay::getInstance();
	Window root = 0, parent = 0;
	Window *children = nullptr;
	unsigned int num_children = 0;

	m_children.clear();
	m_parent = 0;

	const Status res = XQueryTree(
		display, m_win, &root, &parent, &children, &num_children
	);

	if( res != 1 )
	{
		xwmfs_throw(X11Exception(display, res));
	}

	m_parent = parent;

	for( unsigned int i = 0; i < num_children; i++ )
	{
		m_children.insert(children[i]);
	}

	XFree(children);
}

/*
 * explicit template instantiations
 *
 * allow to outline the above template code
 */
template void XWindow::getProperty(const Atom, Property<unsigned long>&) const;
template void XWindow::getProperty(const Atom, Property<std::vector<unsigned long> >&) const;

} // end ns

std::ostream& operator<<(std::ostream &o, const xwmfs::XWindow &w)
{
	const std::ostream::fmtflags f = o.flags();

	o << std::setw(8)
		<< std::hex << std::setfill('0')
		<< std::showbase
		<< w.id()
		<< std::dec << " (" << w.id() << ")";

	o.flags(f);
	o << std::dec;

	return o;
}

