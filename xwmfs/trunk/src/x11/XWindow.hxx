#ifndef XWMFS_WINDOW
#define XWMFS_WINDOW

#include <sstream>
#include <iomanip>

// main xlib header, provides Display declaration
#include "X11/Xlib.h"
// declaration of various atom types like XA_WINDOW
#include "X11/Xatom.h"
// event mask bits
#include "X11/X.h"

// Display wrapper
#include "x11/XDisplay.hxx"
// property helper classes
#include "x11/property.hxx"

namespace xwmfs
{

/**
 * \brief
 *	Wrapper for the X Window primitive
 **/
class XWindow
{
public: // types

	//! Specialized X11Exception for property query errors
	class PropertyQueryError :
		public xwmfs::X11Exception
	{
	public: // functions

		PropertyQueryError(
			const SourceLocation sl,
			Display *dis,
			const int errcode) :
			X11Exception(sl, dis, errcode)
		{
			m_error = m_error + \
				". While trying to get property.";
		}

	};

	//! Specialized X11Exception for property change errors
	class PropertyChangeError :
		public xwmfs::X11Exception
	{
	public: // functions

		PropertyChangeError(
			const SourceLocation sl,
			Display *dis,
			const int errcode) :
			X11Exception(sl, dis, errcode)
		{
			m_error = m_error + \
				". While trying to change property.";
		}
	};
	
	//! Specialized Exception for the case that property types don't match
	class PropertyTypeMismatch :
		public xwmfs::Exception
	{
	public: // functions

		PropertyTypeMismatch(
			const SourceLocation sl,
			Atom expected,
			Atom encountered) :
			Exception(sl,
				"Retrieved property has different type "\
				"than expected: ")
		{
			std::ostringstream s;
			s << "Expected " << expected << " but encountered " \
				<< encountered;
			m_error += s.str();
		}
	};
	
	//! \brief
	//! Specialized Exception for the case that a requested property
	//! doesn't exist
	class PropertyNotExisting :
		public xwmfs::Exception
	{
	public: // functions

		PropertyNotExisting(const SourceLocation sl) :
			Exception(sl, "Requested property is not existing")
		{ }

	};

	//! exception used in situations when an operation is not implemented
	//! by the window manager
	class NotImplemented :
		public xwmfs::Exception
	{
	public: // functions

		NotImplemented(const SourceLocation sl) :
			Exception(sl, "The operation is not implemented")
		{ }
	};

public: // functions

	//! Create an object without binding to a window
	XWindow() :
		m_win(),
		m_event_mask(0),
		m_std_props(StandardProps::instance())
	{ }

	/**
	 * \brief
	 * 	Create an object representing \c win on the current Display
	 **/
	explicit XWindow(Window win);
	
	//! returns true if the object holds a valid XWindow
	bool valid() const { return m_win != 0; }

	//! returns the Xlib primitive Window identifier
	Window id() const { return m_win; }

	/**
	 * \brief
	 *	Retrieve the name of the represented window via EWMH property
	 * \details
	 * 	An X window does not have an integral name attached to it.
	 * 	Instead there are properties the window manager can set
	 * 	according to standards or to its own discretion.
	 *
	 * 	This function tries to retrieve the property as defined by the
	 * 	EWMH standard.
	 *
	 * 	If the name cannot be determined an exception is thrown.
	 **/
	std::string getName() const;

	/**
	 * \brief
	 * 	Retrieve the desktop number the window is currently on
	 * \details
	 * 	The same details found at getName() are true here, too.
	 *
	 * 	If the desktop nr. cannot be determined an exception is
	 * 	thrown.
	 **/
	int getDesktop() const;

	/**
	 * \brief
	 * 	Set \c name as the new name of the current window
	 * \details
	 * 	If the window name cannot be set then an exception is thrown.
	 *
	 * 	On success the window manager should update the visible window
	 * 	title accordingly.
	 **/
	void setName(const std::string &name);

	/**
	 * \brief
	 * 	Set the desktop number \c num as the new desktop for the
	 * 	the current window
	 * \details
	 * 	On error an exception is thrown.
	 *
	 * 	On success the window manager should move the window to the
	 * 	given desktop number.
	 **/
	void setDesktop(const int num);

	/**
	 * \brief
	 *	Retrieve a property from this window object
	 * \details
	 *	The property \c name will be queried from the current window
	 *	and stored in \c p.
	 *
	 *	On error an exception is thrown.
	 **/
	template <typename PROPTYPE>
	inline void getProperty(
		const std::string &name, Property<PROPTYPE> &p) const
	{
		getProperty( XDisplay::getInstance().getAtom(name), p );
	}
	
	/**
	 * \brief
	 *	The same as getProperty(std::string, Property<PROPTYPE>&) but
	 *	for the case when you already have an atom mapping
	 **/
	template <typename PROPTYPE>
	inline void getProperty(
		const Atom name_atom, Property<PROPTYPE> &p
	) const;

	/**
	 * \brief
	 * 	Store a property in this window object
	 * \details
	 * 	Sets the property \c name for the current window to the value
	 * 	stored in \c p.
	 *
	 * 	On error an exception is thrown.
	 **/
	template <typename PROPTYPE>
	inline void setProperty(
		const std::string &name, const Property<PROPTYPE> &p)
	{
		setProperty( XDisplay::getInstance().getAtom(name), p );
	}
	
	/**
	 * \brief
	 * 	The same as setProperty(const std::string&, const
	 * 	Property<PROPTYPE>&) but for the ase when you already have an
	 * 	atom mapping
	 **/
	template <typename PROPTYPE>
	inline void setProperty(
		const Atom name_atom, const Property<PROPTYPE> &p
	);
	
	//! compares the Xlib Window primitive for equality
	bool operator==(const XWindow &o) const { return m_win == o.m_win; }
	
	//! opposive of operator==(const XWindow&) const
	bool operator!=(const XWindow &o) const { return !operator==(o); }

	/**
	 * \brief
	 * 	Inform the X server that we want to be notified of window
	 * 	creation events
	 * \details
	 * 	This call mostly makes sense for the root window to get
	 * 	notified of new windows that come to life.
	 **/
	// XXX should be wisely structured, maybe using a complex Event type
	void selectCreateEvent() const
	{
		// XXX using substructure notify might be unwise. This gets us
		// all the child windows like menus, too.
		// we could also have success only using StructureNotifyMask
		// on exactly the windows we're interested in
		//
		// also: it might be better to look for MappedEvents instead
		// of CreateEvents. In case there are strange hidden windows
		// and such
		m_event_mask |= SubstructureNotifyMask;
		const int res = ::XSelectInput(
			XDisplay::getInstance(), m_win, m_event_mask
		);
		
		// stupid return codes again
		(void)res;
	}

	/**
	 * \brief
	 * 	Inform the X server that we want to be notified of window
	 * 	destruction events
	 * \details
	 * 	If the current window will be destroyed an event will be sent
	 * 	to your X application.
	 **/
	void selectDestroyEvent() const
	{
		m_event_mask |= StructureNotifyMask;
		const int res = ::XSelectInput(
			XDisplay::getInstance(), m_win, m_event_mask
		);
		
		// stupid return codes again
		(void)res;
	}

	/**
	 * \brief
	 * 	Inform the X server that we want to be notified if properties
	 * 	of the current window change
	 * \details
	 * 	This enables you to get events if at the current window
	 * 	properties are added, changed or deleted.
	 **/
	void selectPropertyNotifyEvent() const
	{
		m_event_mask |= PropertyChangeMask;
		const int res = ::XSelectInput(
			XDisplay::getInstance(), m_win, m_event_mask
		);

		// stupid return codes again
		(void)res;
	}

	XWindow& operator=(const XWindow &other)
	{
		m_win = other.m_win;
		m_event_mask  = other.m_event_mask;
		return *this;
	}

private: // data

	//! The X11 window ID this object represents
	Window m_win;

	//! The X11 event mask currently associated with this window
	mutable long m_event_mask;

protected: // data

	const StandardProps &m_std_props;
};


template <typename PROPTYPE>
inline void XWindow::getProperty(
	const Atom name_atom, Property<PROPTYPE> &prop) const
{
	// shorthand for our concrete property object
	typedef Property<PROPTYPE> THIS_PROP;

	Atom x_type = prop.getXType();
	assert( x_type != None );

	Atom actual_type;
	// if 8, 16, or 32-bit format was actually read
	int actual_format;
	unsigned long ret_items;
	unsigned long remaining_bytes;
	unsigned char *data;

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
		// actually present type, format, numbr of items
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
		throw PropertyQueryError(
			XWMFS_SRC_LOCATION,
			XDisplay::getInstance(),
			res
		);
	}

	if( actual_type == None )
	{
		XFree(data);
		throw PropertyNotExisting(XWMFS_SRC_LOCATION);
	}
	else if( x_type != actual_type )
	{
		XFree(data);
		throw PropertyTypeMismatch(
			XWMFS_SRC_LOCATION,
			x_type,
			actual_type);
	}

	assert( actual_format == THIS_PROP::Traits::format );
	assert( ! remaining_bytes );

	// ret_items gives the number of items acc. to actual_format that have
	// been returned
	prop.takeData(data, ret_items * (actual_format / 8));
}

template <typename PROPTYPE>
inline void XWindow::setProperty(
	const Atom name_atom, const Property<PROPTYPE> &prop
)
{
	/*
	 * NOTE: currently only performs mode PropModeReplace
	 *
	 * I don't think that prepend or append are very common use cases.
	 */
	// shorthand for our concrete Property object
	typedef Property<PROPTYPE> THIS_PROP;

	Atom x_type = prop.getXType();
	assert( x_type != None );

	const int siz = THIS_PROP::Traits::getNumElements( prop.get() );

	const int res = XChangeProperty(
		XDisplay::getInstance(),
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
	XDisplay::getInstance().flush();

	// note: on success data is memory allocated by Xlib. data always
	// contains one excess byte
	// that is set to zero thus its possible to use data as a c-string
	// without copying it.
}

} // end ns

//! \brief
//! output operator that prints out the X11 window ID associated with \c
//! w onto the stream in hex and dec
inline std::ostream& operator<<(std::ostream &o, const xwmfs::XWindow &w)
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

#endif // inc. guard

