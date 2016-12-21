#ifndef XWMFS_XDISPLAY_HXX
#define XWMFS_XDISPLAY_HXX

#include <stdint.h>

// main xlib header, provides Display declaration
#include "X11/Xlib.h"
#include "X11/Xatom.h"

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

		AtomMappingError(
			const SourceLocation sl,
			Display *dis,
			const int errcode,
			const std::string &s) :
			X11Exception(sl, dis, errcode)	
		{
			m_error += ". While trying to map " + \
				s + " to a valid atom.";
		}
	};

	//! Specialized Exception for errors regarding opening the Display
	class DisplayOpenError :
		public xwmfs::Exception
	{
	public: // functions

		DisplayOpenError( const SourceLocation sl ) :
			Exception(sl, "Unable to open X11 display: \"")
		{
			m_error += XDisplayName(NULL);
			m_error += "\". ";
			m_error += "Is X running? Is the DISPLAY environment "\
				"variable correct?";
		}
	};

public: // functions

	~XDisplay()
	{
		XCloseDisplay(m_dis);
		m_dis = NULL;
	}

	/**
	 * \brief
	 *	Creates an X atom for the given string and returns it
	 * \details
	 *	The function always returns valid Atom, even if it first
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
			throw AtomMappingError(
				XWMFS_SRC_LOCATION, m_dis, ret, name);

		return ret;
	}
	
	/**
	 * \brief
	 *	Flushes any commands not yet issued to the server
	 * \details
	 * 	Xlib, if not running in synchronous mode, assumes that an X
	 * 	application is frequently sending some X commands to the
	 * 	server and thus buffers command according to some
	 * 	implementation defined strategy.
	 *
	 * 	To make sure that any recently issued communication to the X
	 * 	server takes place right now you can call this function.
	 **/
	void flush()
	{
		XFlush( m_dis );
	}

	//! transparently casts the instance to the Xlib Display primitive
	operator Display*()
	{
		// TODO: check against any side effects through implicit casts
		return m_dis;
	}

	/**
	 * \brief
	 * 	Returns a reference to the single XDisplay instance
	 * \note
	 * 	The first access to this function can generat errors that
	 * 	will be indicated by means of exceptions.
	 **/
	static XDisplay& getInstance()
	{
		static XDisplay dis;

		return dis;
	}

protected: // functions

	//! \brief
	//! Opens the default display, protected due to singleton
	//! pattern
	XDisplay()
	{
		// if NULL is specified, then the value of DISPLAY environment
		// will be used
		m_dis = XOpenDisplay(NULL);

		if( ! m_dis )
		{
			throw DisplayOpenError(XWMFS_SRC_LOCATION);
		}
	}

protected:

	//! The Xlib primitive for the Display
	Display *m_dis;
};

} // end ns

#endif // inc. guard

