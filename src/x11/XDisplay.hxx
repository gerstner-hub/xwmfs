#ifndef XWMFS_XDISPLAY_HXX
#define XWMFS_XDISPLAY_HXX

// C
#include <stdint.h>

// xlib
// main xlib header, provides Display declaration
#include "X11/Xlib.h"
#include "X11/Xatom.h"

// xwmfs
#include "x11/X11Exception.hxx"

namespace xwmfs
{

/**
 * \brief
 * 	Wrapper for Xlib Display type
 * \details
 *	This class associates the Xlib Display type with relevant operations.
 *	Most importantly the Display provides the actual Atom mapping
 *	operations and is also required to create instances of the XWindow
 *	type.
 *
 *	I decided to make this type a singleton as the one and only Display
 *	instance is required at a lot of places and passing it from one place
 *	to another becomes very annoying.
 **/
class XDisplay
{
public: // types

	//! Specialized X11Exception for Atom Mapping Errors
	class AtomMappingError :
		public xwmfs::X11Exception
	{
	public: // functions

		AtomMappingError(Display *dis, const int errcode, const std::string &s);

		XWMFS_EXCEPTION_IMPL;
	};

	//! Specialized Exception for errors regarding opening the Display
	class DisplayOpenError :
		public xwmfs::Exception
	{
	public: // functions

		DisplayOpenError();

		XWMFS_EXCEPTION_IMPL;
	};

public: // functions

	~XDisplay();

	/**
	 * \brief
	 *	Creates an X atom for the given string and returns it
	 * \details
	 *	The function always returns a valid Atom, even if it first
	 *	needs to be created by X11.
	 *
	 *	Can throw AtomMappingError.
	 **/
	Atom getAtom(const std::string &name)
	{
		return getAtom(name.c_str());
	}

	//! see getAtom(const std::string&)
	Atom getAtom(const char *name)
	{
		Atom ret = XInternAtom( m_dis, name, False );

		if( ret == BadAlloc || ret == BadValue || ret == None )
		{
			xwmfs_throw(AtomMappingError(m_dis, ret, name));
		}

		return ret;
	}

	std::string getName(const Atom atom)
	{
		auto str = XGetAtomName( m_dis, atom );
		std::string ret(str);
		XFree(str);
		return ret;
	}

	/**
	 * \brief
	 *	Flushes any commands not yet issued to the server
	 * \details
	 * 	Xlib, if not running in synchronous mode, assumes that an X
	 * 	application is frequently sending some X commands to the
	 * 	server and thus buffers commands according to some
	 * 	implementation defined strategy.
	 *
	 * 	To make sure that any recently issued communication to the X
	 * 	server takes place right now you can call this function.
	 **/
	void flush()
	{
		if( XFlush( m_dis ) == 0 )
		{
			xwmfs_throw( Exception("XFlush failed") );
		}
	}

	/**
	 * \brief
	 * 	Flushes any commands not yet issued to the server and waits
	 * 	for it to process them
	 * \details
	 * 	This is just like flush(), with the extra functionality that
	 * 	this call will block until all requests have also been
	 * 	processed by the XServer. This is helpful, for example, if we
	 * 	have registered new events to be notified of and want to make
	 * 	sure the XServer knows this at some point in time.
	 **/
	void sync()
	{
		if( XSync( m_dis, False ) == 0 )
		{
			xwmfs_throw( Exception("XSync failed") );
		}
	}

	//! transparently casts the instance to the Xlib Display primitive
	operator Display*() { return m_dis; }

	/**
	 * \brief
	 * 	Returns a reference to the single XDisplay instance
	 * \note
	 * 	The first access to this function (construction) can generate
	 * 	errors that will be indicated by means of exceptions.
	 **/
	static XDisplay& getInstance();

protected: // functions

	//! Opens the default display, protected due to singleton
	//! pattern
	XDisplay();

	// disallow copy of the singleton XDisplay
	XDisplay(const XDisplay &other) = delete;

protected:

	//! The Xlib primitive for the Display
	Display *m_dis = nullptr;
};

} // end ns

#endif // inc. guard

