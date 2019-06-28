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

std::string XWindow::getCommand() const
{
	xwmfs::Property<const char *> name;

	this->getProperty(m_std_props.atom_icccm_wm_command, name);

	return name.get();
}

std::string XWindow::getLocale() const
{
	xwmfs::Property<const char *> locale;

	this->getProperty(m_std_props.atom_icccm_wm_locale, locale);

	return locale.get();
}

Window XWindow::getClientLeader() const
{
	xwmfs::Property<Window> leader;

	this->getProperty(m_std_props.atom_icccm_wm_client_leader, leader);

	return leader.get();
}

Atom XWindow::getWindowType() const
{
	xwmfs::Property<XAtom> type;

	this->getProperty(m_std_props.atom_ewmh_wm_window_type, type);

	return type.get();
}

void XWindow::getProtocols(AtomVector &protocols) const
{
	protocols.clear();

	Atom *ret = nullptr;
	int ret_count = 0;

	const auto status = XGetWMProtocols(
		XDisplay::getInstance(),
		m_win,
		&ret,
		&ret_count
	);

	if( status == 0 )
	{
		xwmfs_throw(X11Exception(XDisplay::getInstance(), status));
	}

	for( int num = 0; num < ret_count; num++ )
	{
		protocols.push_back(ret[num]);
	}

	XFree(ret);
}

XWindow::ClassStringPair XWindow::getClass() const
{
	/*
	 * there's a special pair of functions X{Set,Get}ClassHint but that
	 * would be more work for us, we get the raw property which consists
	 * of two consecutive null terminated strings
	 */
	xwmfs::Property<const char *> clazz;

	this->getProperty(m_std_props.atom_icccm_wm_class, clazz);

	ClassStringPair ret;

	ret.first = std::string( clazz.get() );
	// this is not very safe but our Property modelling currently lacks
	// support for strings containing null terminators (we can't get the
	// complete size from the property)
	ret.second = std::string( clazz.get() + ret.first.length() + 1 );

	return ret;
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

Window XWindow::createChild()
{
	auto &display = XDisplay::getInstance();

	Window new_win = XCreateSimpleWindow(
		display,
		this->id(),
		// dimensions and alike don't matter for this hidden window
		-10, -10, 1, 1, 0, 0, 0
	);

	if( new_win == 0 )
	{
		xwmfs_throw(
			xwmfs::Exception("Failed to create pseudo child window")
		);
	}

	display.flush();

	return new_win;
}

void XWindow::convertSelection(
	const XAtom &selection,
	const XAtom &target_type,
	const XAtom &target_prop
)
{
	auto &display = XDisplay::getInstance();

	if( XConvertSelection(
		display,
		selection,
		target_type,
		target_prop,
		m_win,
		CurrentTime
	) != 1 )
	{
		xwmfs_throw(
			xwmfs::Exception("Failed to request selecton conversion")
		);
	}

	display.flush();
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

	event.xclient.type = ClientMessage;
	event.xclient.serial = 0;
	event.xclient.send_event = True;
	event.xclient.message_type = message;
	event.xclient.window = window ? window->id() : 0;
	event.xclient.format = 32;
	std::memcpy(event.xclient.data.b, data, len);

	sendEvent(event);
}

void XWindow::sendEvent(const XEvent &event)
{
	auto &display = XDisplay::getInstance();

	const Status s = XSendEvent(
		display,
		this->id(),
		False,
		m_send_event_mask,
		const_cast<XEvent*>(&event)
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

void XWindow::getPropertyList(AtomVector &atoms)
{
	atoms.clear();

	int num_atoms = 0;

	Atom *list = XListProperties(
		XDisplay::getInstance(),
		m_win,
		&num_atoms
	);

	if( list == nullptr )
	{
		// could be an error (probably) or a window without any
		// properties
		return;
	}

	for( int i = 0; i < num_atoms; i++ )
	{
		atoms.push_back( list[i] );
	}

	XFree(list);
}

void XWindow::getPropertyInfo(const XAtom &property, PropertyInfo &info)
{
	int actual_format = 0;
	unsigned long number_items = 0;
	/*
	 * we don't want any data returned, but the function excepts valid
	 * pointers to pointers here, otherwise we segfault
	 */
	unsigned long bytes_left = 0;
	unsigned char *prop_data = nullptr;

	const auto res = XGetWindowProperty(
		XDisplay::getInstance(),
		m_win,
		property,
		0, /* offset */
		0, /* length */
		False, /* deleted property ? */
		AnyPropertyType,
		&info.type,
		&actual_format,
		&number_items,
		&bytes_left, /* bytes left to read */
		&prop_data /* output buffer to read into */
	);

	if( res != Success )
	{
		xwmfs_throw(X11Exception(XDisplay::getInstance(), res));
	}

	info.items = bytes_left / (actual_format / 8);
	info.format = actual_format;

	XFree(prop_data);
}

template <typename PROPTYPE>
void XWindow::getProperty(
	const Atom name_atom,
	Property<PROPTYPE> &prop,
	const PropertyInfo *info
) const
{
	// shorthand for our concrete property object
	typedef Property<PROPTYPE> THIS_PROP;

	Atom x_type = THIS_PROP::getXType();
	assert(x_type != None);

	Atom actual_type;
	// if 8, 16, or 32-bit format was actually read
	int actual_format = 0;
	unsigned long ret_items = 0;
	unsigned long remaining_bytes = 0;
	unsigned char *data = nullptr;

	const size_t max_len = info ?
		(info->items * (info->format / 8)) : 65536 / 4;

	const int res = XGetWindowProperty(
		XDisplay::getInstance(),
		m_win,
		name_atom,
		// offset into the property data
		0,
		// maximum length of the property to read in 32-bit items
		max_len,
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

	try
	{
		if( actual_type == None )
		{
			xwmfs_throw(PropertyNotExisting());
		}
		else if( x_type != actual_type )
		{
			xwmfs_throw(PropertyTypeMismatch(x_type, actual_type));
		}
		else if( remaining_bytes != 0 )
		{
			xwmfs_throw(Exception("Bytes remaining during property read"));
		}

		assert( actual_format == THIS_PROP::Traits::format );

		// ret_items gives the number of items acc. to actual_format that have
		// been returned
		prop.takeData(data, ret_items * (actual_format / 8));
	}
	catch( ... )
	{
		XFree(data);
		throw;
	}
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

void XWindow::delProperty(const Atom name_atom)
{
	auto &display = XDisplay::getInstance();

	const auto status = XDeleteProperty(display, m_win, name_atom);

	if( status == 0 )
	{
		xwmfs_throw(X11Exception(display, status));
	}

	// see setProperty()
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

void XWindow::moveResize(const XWindowAttrs &attrs)
{
	auto &display = XDisplay::getInstance();
	const auto status = XMoveResizeWindow(
		display, m_win, attrs.x, attrs.y, attrs.width, attrs.height
	);

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
template void XWindow::getProperty(const Atom, Property<unsigned long>&, const PropertyInfo*) const;
template void XWindow::getProperty(const Atom, Property<const char*>&, const PropertyInfo*) const;
template void XWindow::getProperty(const Atom, Property<std::vector<XAtom> >&, const PropertyInfo*) const;
template void XWindow::getProperty(const Atom, Property<std::vector<unsigned long> >&, const PropertyInfo*) const;
template void XWindow::getProperty(const Atom, Property<std::vector<int> >&, const PropertyInfo*) const;
template void XWindow::setProperty(const Atom, const Property<const char*>&);
template void XWindow::setProperty(const Atom, const Property<int>&);
template void XWindow::setProperty(const Atom, const Property<utf8_string>&);

} // end ns

std::ostream& operator<<(std::ostream &o, const xwmfs::XWindow &w)
{
	const std::ostream::fmtflags f = o.flags();

	o << "0x" << std::setw(8)
		<< std::hex << std::setfill('0')
		<< w.id()
		<< std::dec << " (" << w.id() << ")";

	o.flags(f);
	o << std::dec;

	return o;
}

