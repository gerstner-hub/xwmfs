#ifndef XWMFS_XATOM_HXX
#define XWMFS_XATOM_HXX

#include <map>
#include <string>
#include <stdint.h>

#include "common/RWLock.hxx"

#include "x11/XDisplay.hxx"
#include "x11/X11Exception.hxx"

namespace xwmfs
{

/**
 * \brief
 * 	Wrapper for the Xlib C-type 'Atom'
 * \details
 * 	This class mainly wraps the Xlib Atom primitive. It is type safe and
 * 	in theory we can add some smart operations in the future.
 *
 * 	The class provides a conversion operator that allows to cast the type
 * 	directly to the Xlib C-type if required.
 * \note
 * 	In the Xlib world we have Atoms which are unique identifiers for:
 *
 * 	- property names
 * 	- property types
 *
 * 	Properties are attached to windows. For example WM_NAME is the name of
 * 	a window property that contains a STRING data type that makes up the
 * 	title of a window.
 *
 * 	The property names and types are Latin1 encoded strings (at least the
 * 	manual says so). Xlib provides Atoms as alternative for these strings.
 * 	This is for efficiency reasons as it is cheaper then passing strings
 * 	around.
 *
 * 	In the Xlib the Atom type is some integer-like type that doesn't
 * 	provide much type safety. Thus I introduced this C++ type that wraps
 * 	the C type.
 **/
class XAtom
{
public: // functions

	XAtom() :
		m_atom(None)
	{ }

	XAtom( const XAtom &atom ) :
		m_atom(atom.m_atom)
	{ }

	XAtom( const Atom &atom ) :
		m_atom(atom)
	{ }

	XAtom& operator=(const Atom &atom) 
	{
		m_atom = atom;
		return *this;
	}

	XAtom& operator=(const XAtom &atom)
	{
		m_atom = atom.m_atom;
		return *this;
	}

	// TODO: check against any side-effects with implicit casting
	operator Atom() const
	{
		return m_atom;
	}

	Atom get() const { return m_atom; }

protected: // data

	Atom m_atom;
};

/**
 * \brief
 * 	Efficient caching of property name/type to atom mappings
 * \details
 * 	This class allows to cache Xlib atom mappings that can be quickly
 * 	retrieved in the future. If a mapping is not cached already then it is
 * 	retrieved via Xlib.
 *
 * 	This class is thread safe. Read-access can occur in parallel, write
 * 	access (due to cache misses) can only occur when one client is
 * 	presently accessing the cache.
 *
 * 	This class is currently implemented as a singleton type. Get the
 * 	global instance of the mapper via the getInstance() function.
 * \note
 * 	There are Xlib functions that allow to map strings to atoms and vice
 * 	versa. However I'm not yet sure how efficient that mapping is done.
 * 	One should assume that any mappings established should be efficiently
 * 	cached locally within Xlib. Thus when we request the same mapping
 * 	again the lookup will be very fast.
 *
 * 	I decided to provide an own caching facility, however, to be on the
 * 	safe side. Also the interface of this class allows for more C++-like
 * 	usage.
 **/
class XAtomMapper
{
public: // functions

	//! Retrieves the global mapper instance
	static XAtomMapper& getInstance()
	{
		static XAtomMapper inst;

		return inst;
	}

	/**
	 * \brief
	 * 	Returns the atom representation of the given property name
	 * \details
	 * 	The function performs caching of atom values once resolved to
	 * 	avoid having to excessively talk to the XServer
	 **/
	XAtom getAtom(const std::string &s);

	//! Wrapper for getAtom(const std::string &) that uses const char*
	XAtom getAtom(const char *s)
	{
		return this->getAtom(std::string(s));
	}

protected: // functions

	//! protected constructor to enforce singleton usage
	XAtomMapper() :
		m_mappings()
	{ }

protected: // types

	//! The map contained that maps string to X atoms
	typedef std::map< std::string, Atom > AtomMapping;

protected: // data

	//! contains the actual mappings
	AtomMapping m_mappings;
	//! synchronizes parallel read and update of \c m_mappings
	RWLock m_mappings_lock;
};

/**
 * \brief
 * 	A struct-like class to hold a number of default property names
 * \details
 * 	These properties are statically referenced in the code. We don't want
 * 	to write them redundantly time and again so we rather use these global
 * 	constants.
 *
 * 	It's a singleton class to defer the time of instantiation to after the
 * 	entry of the main() function to avoid static initialization issues.
 */
struct StandardProps
{
	//! window name property (EWMH)
	XAtom atom_ewmh_window_name;
	//! desktop on which a window is currently on
	XAtom atom_ewmh_window_desktop;
	//! name of UTF8 string type
	XAtom atom_ewmh_utf8_string;
	//! property of EWMH comp. wm on root window
	XAtom atom_ewmh_support_check;
	//! property containing EWMH comp. wm PID
	XAtom atom_ewmh_wm_pid;
	//! property indicating that EWMH comp. wm is set to show the desktop
	XAtom atom_ewmh_wm_desktop_shown;
	//! property giving the number of desktops available
	XAtom atom_ewmh_wm_nr_desktops;
	//! property denoting the currently active desktop number
	XAtom atom_ewmh_wm_cur_desktop;
	//! property denoting EWMH comp. wm desktop number for a window
	XAtom atom_ewmh_desktop_nr;
	//! property containing an array of windows managed by EWMH comp. WM
	XAtom atom_ewmh_wm_window_list;
	//! name of the machine running the client as seen from server
	XAtom atom_icccm_client_machine;
	//! window name property acc. to ICCCM spec.
	XAtom atom_icccm_window_name;

	static const StandardProps& instance();

private:

	StandardProps();
};

} // end ns

#endif // inc. guard

