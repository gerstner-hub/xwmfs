#ifndef XWMFS_PROPERTY
#define XWMFS_PROPERTY

#include <stdio.h>
#include <cstring>
#include <vector>

#include "x11/XDisplay.hxx"
#include "x11/XAtom.hxx"

namespace xwmfs
{

/**
 * \brief
 * 	Type traits for X properties
 * \details
 * 	This is a generic template class for getting and setting X properties.
 *
 * 	This generic template is not intended for use but only explicit
 * 	template specialications for concrete types.
 *
 * 	The purpose of the traits are to be used together with the
 * 	Property class to smartly send/receive data of a certain format
 * 	to/from the X-server.
 *
 * 	The X11 protocol uses RPC type definitions that need to be specified
 * 	at the Xlib's C-interface by means of setting the right types,
 * 	pointers, sizes etc. This is pretty low-level and unattractive to use.
 *
 * 	These traits and the Property class help to make these
 * 	operations more type safe, better readable and less redundant.
 *
 * 	The approach is that the traits and Property types are instantiated
 * 	using the C or C++ types that are locally used in the program. Then
 * 	when we need to retrieve or send that type from/to the X server the
 * 	Property class together with the traits types know internally how to
 * 	transform the local data into data the X server understands and vice
 * 	versa.
 * \note
 * 	This generic template declaration contains intentionally ridiculous
 * 	constants such that nothing sensible can be done with it if
 * 	accidentally instantiatied.
 * \todo
 * 	XXX
 * 	This whole property business needs a serious work over. It was started
 * 	with a "getting data from the server" approach. And the "sending data
 * 	to the server" was not very well thought off.
 **/
template <typename PROPTYPE>
class XPropTraits
{
public: // constants

	//! \brief
	//! this is the Xlib atom data type corresponding to PROPTYPE that is
	//! passed to the X functions for identification
	static const Atom x_type = XA_CARDINAL;
	//! \brief
	//! if PROPTYPE has got a fixed size then this constants denotes that
	//! size in bytes, otherwise set to zero
	static const unsigned long fixed_size = 5;
	//! \brief
	//! A pointer to PROPTYPE that can be passed to the X functions
	//! for retrieving / passing data
	typedef float* NativePtrType;
	//! \brief
	//! the format in X terms, determines the width of a single sequence
	//! item in bits (e.g. for arrays of 8-bit, 16-bit or 32-bit items)
	static const char format = 31;

public: // functions
	
	/**
	 * \brief
	 * 	Assign a new value to the native PROPTYPE from the given raw X
	 * 	data
	 * \details
	 * 	The contract of this function is as follows:
	 *
	 * 	The first parameter is an out parameter, a reference to
	 * 	PROPTYPE, where the function should store the new value for
	 * 	PROPTYPE.
	 *
	 * 	The new value for PROPTYPE is provided in the second parameter
	 * 	which is a pointer to the raw X data received. The third
	 * 	parameter determines the number of sequence items that can be
	 * 	found in the second parameter, if applicable.
	 **/
	static void assignNative(int &i, NativePtrType data, unsigned int count)
	{ 
		// if you reach this code, you're screwed. No traits for your
		// type...
		class no_traits_for_your_type;
		int s;

		no_traits_for_your_type *f =
			static_cast<no_traits_for_your_type*>(&s);
	}
};

template <>
//! property trait specialization for integers
class XPropTraits<int>
{
public: // constants

	static const Atom x_type = XA_CARDINAL;
	static const unsigned long fixed_size = 4;
	typedef long* NativePtrType;
	static const char format = 32;

public: // functions

	static void assignNative(
		int &i,
		NativePtrType data,
		unsigned int count
	)
	{
		i = static_cast<int>(*data);
		(void)count;
	}

	//! determines whether \c data points to \c native
	static bool samePtrs(int &native, long *&data)
	{
		return &native == (int*&)data;
	}
	
	//! \brief
	//! returns the number of elements present in \c s (in X terminology,
	//! acc. to format)
	static int getNumElements(const int &s)
	{
		(void)s;
		return 1;
	}

	/**
	 * \brief
	 * 	Fills the given raw X data \c data from the current value in
	 * 	the native PROPTYPE
	 **/
	static void assignData(const int &s, NativePtrType &data)
	{
		// We simply set the pointer to the PROPTYPE item as a flat
		// copy.
		data = (NativePtrType)&s;
	}
};

template <>
//! property type specialization for strings
class XPropTraits<const char*>
{
public: // constants

	static const Atom x_type = XA_STRING;
	static const unsigned long fixed_size = 0;
	static const char format = 8;
	typedef const char* NativePtrType;

public: // functions

	static void assignNative(
		const char * & s, NativePtrType data, unsigned int count)
	{
		(void)count;
		s = data;
	}

	static void assignData(const char * & s, NativePtrType &data)
	{
		data = s;
	}

	static bool samePtrs(const char * &native, const char * &data)
	{
		return native == data;
	}
	
	static int getNumElements(const char* const s)
	{
		// strings in X are transferred without null terminator, the
		// library always provides a terminating null byte in
		// transferred data
		return s ? std::strlen(s) : 0;
	}
};

template <>
//! property type specialization for Window identifiers
class XPropTraits<Window>
{
public: // constants

	static const Atom x_type = XA_WINDOW;
	static const unsigned long fixed_size = 4;
	static const char format = 32;
	typedef long* NativePtrType;

public: // functions

	static void assignNative(
		Window &w, NativePtrType data, unsigned int count)
	{
		(void)count;
		w = static_cast<Window>(*data);
	}

	static bool samePtrs(Window &native, long *data)
	{
		return &native == (Window*)data;
	}
};

template <>
//! property type specialization for arrays of Window identifiers
class XPropTraits< std::vector<Window> >
{
public: // constants

	static const Atom x_type = XA_WINDOW;
	static const unsigned long fixed_size = 0;
	static const char format = 32;
	typedef long* NativePtrType;

public: // functions
		
	static void assignNative(
		std::vector<Window> &w,
		NativePtrType data,
		unsigned int count)
	{
		for( unsigned int e = 0; e < count; e++ )
		{
			w.push_back(data[e]);
		}
	}
	
	static bool samePtrs(
		std::vector<Window> &native, long *data)
	{
		return native.data() == (Window*)data;
	}
};

/**
 * \brief
 * 	A type used for differentiation between a plain ASCII string and an
 * 	Xlib utf8 string
 * \details
 * 	It is currently just a contained for a char pointer.
 * \note
 *	The X type UTF8_STRING is a new type proposed in X.org but not yet
 *	really approved:
 *
 *	http://www.pps.jussieu.fr/~jch/software/UTF8_STRING/UTF8_STRING.text
 *
 *	UTF8_STRING returns 8-bit encoded UTF8-Data without a NULL-terminator.
 *
 *	This type is used by the EWMH standard, so we need to be able to deal
 *	with it.
 **/
struct utf8_string
{
public: // functions

	utf8_string() : data()
	{ };

public: // data

	char *data;
};

template <>
//! property type specialization for utf8 strings
class XPropTraits<utf8_string>
{
public: // constants

	static XAtom x_type;
	static const unsigned long fixed_size = 0;
	static const char format = 8;
	typedef char* NativePtrType;

public: // functions

	static void init()
	{
		// this XLib property type atom is not available as a constant
		// in the Xlib headers but needs to be queried at runtime.
		x_type = StandardProps::instance().atom_ewmh_utf8_string;
	}

	static void assignNative(
		utf8_string &s, NativePtrType data, unsigned int count)
	{
		(void)count;
		s.data = data;
	}

	static int getNumElements(utf8_string &s)
	{
		return s.data ? std::strlen(s.data) : 0;
	}
	
	static bool samePtrs( utf8_string &native, char *data)
	{
		return native.data == data;
	}
};

/**
 * \brief
 *	X11 property representation
 * \details
 * 	Based on the XPropTraits definitions above this class allows to
 * 	have C++ Property objects that can get and set data
 * 	transparently from/to the X server and transform the data from
 * 	the native C++ world into the X world and vice versa.
 * \todo
 * 	XXX we need to check what happens if a Property instance is copied.
 * 	Due to checkDelete() handling we might end up with invalid references.
 * 	Either introduce reference counting or restrict copying.
 **/
template <typename PROPTYPE>
class Property
{
	// The XWindow class is actually doing the communication via Xlib so
	// it needs to mess with the Property internals
	friend class XWindow;

public: // types

	//! the matching traits for our property type
	typedef XPropTraits<PROPTYPE> Traits;
	//! The correct pointer type for our property type
	typedef typename XPropTraits<PROPTYPE>::NativePtrType NativePtrType;

public: // functions

	//! construct an empty/default property value
	Property() :
		m_native(),
		m_data(nullptr)
	{ }

	//! construct a property holding the value from \c p
	Property(const PROPTYPE &p) :
		m_native(),
		m_data()
	{
		// our own assignment operator knows how to deal with this
		*this = p;
	}

	//! frees unneeded memory, if required
	~Property()
	{
		checkDelete();
	}

	/**
	 * \brief
	 * 	Retrieves a reference to the currently stored property value
	 * \details
	 * 	If there happens to be no valid data stored then an exception
	 * 	is thrown.
	 **/
	const PROPTYPE& get() const
	{
		if( !m_data )
		{
			throw Exception(
				XWMFS_SRC_LOCATION,
				"No valid property stored"
			); 
		}

		return m_native;
	}

	/**
	 * \brief
	 * 	Retrieves a pointer to the raw data associated with the
	 * 	property
	 **/
	typename Traits::NativePtrType getRawData() const
	{
		return m_data;
	}

	//! returns whether valid property data is set
	bool valid() const
	{
		return m_data != NULL;
	}

	/**
	 * \brief
	 * 	Assigns the given property value from \c p
	 **/
	Property& operator=(const PROPTYPE &p)
	{
		checkDelete();

		m_native = p;
		Traits::assignData( m_native, m_data );

		return *this;
	}

protected: // functions

	/**
	 * \brief
	 * 	Set the current value of the stored native PROPTYPE from the
	 * 	given X data found in \c data
	 * \details
	 * 	\c data is a pointer to the data received from Xlib. It needs
	 * 	to be freed at an appropriate time via XFree().
	 *
	 * 	\c size determines the number of bytes present in \c data.
	 **/
	void takeData(unsigned char *data, unsigned long size)
	{
		checkDelete();

		if( Traits::fixed_size )
		{
			assert( size == Traits::fixed_size );
		}

		m_data = reinterpret_cast<NativePtrType>(data);
		Traits::assignNative(
			m_native,
			m_data,
			size / (Traits::format / 8)
		);
	}
	
	//! Retrieves the associated XAtom type from the traits of PROPTYPE
	Atom getXType() const { return Traits::x_type; }

	/**
	 * \brief
	 * 	If the current Property instance contains data allocated by
	 * 	Xlib then it is deleted
	 **/
	void checkDelete()
	{
		// second condition checks that m_data really is allocated by
		// X and not a pointer to m_native
		if( m_data && !Traits::samePtrs(m_native, m_data) )
		{
			// note: XFree returns strange codes ...

			/*const int res = */XFree( (void*)m_data);
			// what is that res ?!
			//std::cout << "Xfree res = " << res << std::endl;
			//assert( res == Success );

			m_data = nullptr;
		}
	}
private:
	//! The native property type
	PROPTYPE m_native;
	//! \brief
	//! Either a pointer to m_native that can be fed to Xlib or a pointer
	//! to data received from Xlib, from which m_native is build from
	typename Traits::NativePtrType m_data;
};

} // end ns

#endif // inc. guard

