#ifndef XWMFS_WINDOW_HXX
#define XWMFS_WINDOW_HXX

// C++
#include <set>
#include <iosfwd>
#include <vector>
#include <utility>

#include <unistd.h> // pid_t

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

class XWindowAttrs;

/**
 * \brief
 *	Wrapper for the X Window primitive
 * \details
 * 	This class stores an XWindow identifier and provides operation on X
 * 	Window objects, like retrieving and setting window properties.
 **/
class XWindow
{
public: // types

	//! Specialized X11Exception for property query errors
	class PropertyQueryError :
		public xwmfs::X11Exception
	{
	public: // functions

		PropertyQueryError(Display *dis, const int errcode);

		XWMFS_EXCEPTION_IMPL;
	};

	//! Specialized X11Exception for property change errors
	class PropertyChangeError :
		public xwmfs::X11Exception
	{
	public: // functions

		PropertyChangeError(Display *dis, const int errcode);

		XWMFS_EXCEPTION_IMPL;
	};

	//! Specialized Exception for the case that property types don't match
	class PropertyTypeMismatch :
		public xwmfs::Exception
	{
	public: // functions

		PropertyTypeMismatch(Atom expected, Atom encountered);

		XWMFS_EXCEPTION_IMPL;
	};

	//! \brief
	//! Specialized Exception for the case that a requested property
	//! doesn't exist
	class PropertyNotExisting :
		public xwmfs::Exception
	{
	public: // functions

		PropertyNotExisting() :
			Exception("Requested property is not existing")
		{ }

		XWMFS_EXCEPTION_IMPL;
	};

	//! exception used in situations when an operation is not implemented
	//! by the window manager
	class NotImplemented :
		public xwmfs::Exception
	{
	public: // functions

		NotImplemented() :
			Exception("The operation is not implemented")
		{ }

		XWMFS_EXCEPTION_IMPL;
	};

	/**
	 * \brief
	 * 	Keeps metadata about a property
	 **/
	struct PropertyInfo
	{
		//! the property's type
		Atom type = None;
		//! the number of items of the given format
		size_t items = 0;
		//! the format of the property 8/16/32
		size_t format = 0;
	};

	typedef std::set<Window> WindowSet;

	typedef std::vector<Atom> AtomVector;

	typedef std::pair<std::string, std::string> ClassStringPair;

public: // functions

	//! Create an object without binding to a window
	XWindow() :
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

	//! string representation of id()
	std::string idStr() const;

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
	 * 	Retrieve the PID that owns the represented window
	 * \details
	 * 	If the pid cannot be determined an exception is thrown.
	 **/
	pid_t getPID() const;

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

	//! returns the client machine the window is associated with
	std::string getClientMachine() const;

	/**
	 * \brief
	 * 	Returns the window class parameters for this window
	 * \details
	 * 	The first returned string is the name of the application, the
	 * 	second returned string is the class of the application.
	 *
	 * 	On error an exception is thrown.
	 **/
	ClassStringPair getClass() const;

	/**
	 * \brief
	 * 	Requests the X server to destroy the represented window and
	 * 	all sub-windows
	 * \details
	 * 	This request cannot be ignored by the application owning the
	 * 	window. It is a forcible way to remove the window from the X
	 * 	server. An alternative is the function sendDeleteRequest()
	 * 	which is a cooperative way to ask an application to close the
	 * 	window.
	 *
	 * 	The window represented by this object will be invalid after a
	 * 	successful return from this function. Further operations on it
	 * 	will fail.
	 **/
	void destroy();

	/**
	 * \brief
	 * 	Requests the targeted window to close itself
	 * \details
	 * 	In contrast to destroy() this is a cooperative call that
	 * 	allows the targeted window to cleanly close itself and the
	 * 	associated application, possibly asking the user first.
	 **/
	void sendDeleteRequest();

	/**
	 * \brief
	 * 	Retrieves a list of all properties currently present on this
	 * 	window
	 * \details
	 * 	The vector of \c atoms is cleared in any case and will be
	 * 	filled with the atoms identifying the properties existing on
	 * 	this window.
	 **/
	void getPropertyList(AtomVector &atoms);

	/**
	 * \brief
	 * 	Retrieves property metadata about the given property present
	 * 	on this window
	 * \details
	 * 	This information is useful for processing properties in a
	 * 	generic way without knowing their type in advance. For example
	 * 	to be used together with getPropertyList().
	 **/
	void getPropertyInfo(const XAtom &property, PropertyInfo &info);

	/**
	 * \brief
	 *	Retrieve a property from this window object
	 * \details
	 *	The property \c name will be queried from the current window
	 *	and stored in \c p.
	 *
	 *	On error an exception is thrown.
	 *
	 *	The PROPTYPE type must match the property's type.
	 **/
	template <typename PROPTYPE>
	void getProperty(const std::string &name, Property<PROPTYPE> &p) const
	{
		getProperty( XDisplay::getInstance().getAtom(name), p );
	}

	/**
	 * \brief
	 *	The same as getProperty(std::string, Property<PROPTYPE>&) but
	 *	for the case when you already have an atom mapping
	 * \param[in] info
	 * 	An optional pointer to the PropertyInfo for the given atom for
	 * 	helping determining the correct size to retrieve
	 **/
	template <typename PROPTYPE>
	void getProperty(
		const Atom name_atom,
		Property<PROPTYPE> &p,
		const PropertyInfo *info = nullptr
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
	void setProperty(const std::string &name, const Property<PROPTYPE> &p)
	{
		setProperty( XDisplay::getInstance().getAtom(name), p );
	}

	/**
	 * \brief
	 * 	The same as
	 * 
	 * 	setProperty(const std::string&, const Property<PROPTYPE>&)
	 *
	 * 	but for the case when you already have an atom mapping for the
	 * 	property name
	 **/
	template <typename PROPTYPE>
	void setProperty(const Atom name_atom, const Property<PROPTYPE> &p);

	//! compares the Xlib Window primitive for equality
	bool operator==(const XWindow &o) const { return m_win == o.m_win; }

	//! opposite of operator==(const XWindow&) const
	bool operator!=(const XWindow &o) const { return !operator==(o); }

	/**
	 * \brief
	 * 	Inform the X server that we want to be notified of window
	 * 	creation events
	 * \details
	 * 	This call mostly makes sense for the root window to get
	 * 	notified of new windows that come to life.
	 **/
	void selectCreateEvent() const
	{
		// This is the only way to get CreateNotify events from the X
		// server.
		//
		// This gets us events for all the child windows like menus,
		// too.
		//
		// Thus if we don't want grandchildren Windows of the root
		// window then we need to filter on the event receiving side
		// within our process
		selectEvent(SubstructureNotifyMask);
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
		selectEvent(StructureNotifyMask);
	}

	/**
	 * \brief
	 * 	Inform the X server that we want to be notified if properties
	 * 	of the current window change
	 * \details
	 * 	This enables you to get events if properties of the current
	 * 	window are added, changed or deleted.
	 **/
	void selectPropertyNotifyEvent() const
	{
		selectEvent(PropertyChangeMask);
	}

	XWindow& operator=(const XWindow &other)
	{
		m_win = other.m_win;
		m_parent = other.m_parent;
		m_input_event_mask = other.m_input_event_mask;
		m_send_event_mask = other.m_send_event_mask;
		return *this;
	}

	/**
	 * \brief
	 * 	Retrieve the attributes for this window
	 * \details
	 * 	If this fails then an X11Exception is thrown
	 **/
	void getAttrs(XWindowAttrs &attrs);

	void setParent(Window parent) { m_parent = parent; }
	void setParent(const XWindow &parent) { setParent(parent.id()); }

	Window getParent() const { return m_parent; }

	const WindowSet& getChildren() const { return m_children; }

	void addChild(const XWindow &child) { m_children.insert(child.id()); }
	void delChild(const XWindow &child) { m_children.erase(child.id()); }

	//! queries parent and child windows of this window and sets them acc.
	void updateFamily();

protected: // functions

	/**
	 * \brief
	 * 	Adds the given event(s) to the set of events we want to be
	 * 	notified if they occur for the current window
	 **/
	void selectEvent(const long new_event) const;

	/**
	 * \brief
	 * 	Sends a request to the window with a single long parameter as
	 * 	data
	 **/
	void sendRequest(
		const XAtom &message,
		long data,
		const XWindow *window = nullptr
	)
	{
		return sendRequest(
			message,
			(const char*)&data,
			sizeof(data),
			window
		);
	}

	/**
	 * \brief
	 * 	Sends a request to the window
	 * \details
	 * 	To have the window (manager) actively do something on our
	 * 	request we need to send it an event.
	 *
	 * 	This event contains a message of what to do and parameters to
	 * 	that message.
	 *
	 * 	Throws an exception on error.
	 * \param[in] message
	 * 	The basic message type that defines the further parameters of
	 * 	the request
	 * \param[in] data
	 * 	The raw data making up the parameters to the message
	 * \param[in] len
	 * 	The length of the raw data in bytes
	 * \param[in] window
	 * 	An optional window parameter. Mostly used for the RootWin to
	 * 	specify another window upon which should be acted.
	 **/
	void sendRequest(
		const XAtom &message,
		const char *data = nullptr,
		const size_t len = 0,
		const XWindow *window = nullptr
	);

protected: // data

	//! The X11 window ID this object represents
	Window m_win = 0;
	//! The X11 window ID of the parent of this window
	Window m_parent = 0;
	//! A set of X11 window IDs that presents the children of this window
	WindowSet m_children;

	//! The X11 input event mask currently associated with this window
	mutable long m_input_event_mask = 0;
	//! The X11 send event mask currently associated with this window
	mutable long m_send_event_mask = NoEventMask;

	const StandardProps &m_std_props;
};

} // end ns

//! \brief
//! output operator that prints out the X11 window ID associated with \c
//! w onto the stream in hex and dec
std::ostream& operator<<(std::ostream &o, const xwmfs::XWindow &w);

#endif // inc. guard

