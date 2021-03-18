#ifndef XWMFS_PROPERTY
#define XWMFS_PROPERTY

// C++
#include <cstring>
#include <vector>

// xwmfs
#include "x11/XDisplay.hxx"
#include "x11/XAtom.hxx"
#include "x11/utf8_string.hxx"

namespace xwmfs
{

/**
 * \brief
 * 	Type traits for X properties
 * \details
 * 	This is a generic template class for getting and setting X properties.
 *
 * 	This generic template is not intended for use but only explicit
 * 	template specialications for concrete types should be used. This
 * 	generic template merely serves the purpose of documentation.
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
	//! if PROPTYPE has got a fixed size then this constant denotes that
	//! size in bytes, otherwise set to zero
	static const unsigned long fixed_size = 0;
	//! \brief
	//! A pointer to PROPTYPE that can be passed to the X functions
	//! for retrieving / passing data
	typedef float* XPtrType;
	//! \brief
	//! the format in X terms, determines the width of a single sequence
	//! item in bits (e.g. for arrays of 8-bit, 16-bit or 32-bit items)
	static const char format = 0;

public: // functions

	/**
	 * \brief
	 * 	Returns the number of elements the given property has in X
	 * 	terms
	 **/
	static
	int getNumElements(const PROPTYPE &val) { return 0; }

	/**
	 * \brief
	 * 	Set the current value of the native PROPTYPE from the given
	 * 	raw X data
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
	static
	void x2native(PROPTYPE &i, XPtrType data, unsigned int count)
	{}

	/**
	 * \brief
	 * 	Transform the current value of the native PROPTYPE into raw X
	 * 	data
	 **/
	static
	void native2x(const PROPTYPE &s, XPtrType &data)
	{}

	// never instantiate this type
	XPropTraits() = delete;
};

template <>
//! property trait specialization for integers
class XPropTraits<int>
{
public: // constants

	static const Atom x_type = XA_CARDINAL;
	static const unsigned long fixed_size = sizeof(int);
	typedef long* XPtrType;
	static const char format = 32;

public: // functions

	static
	int getNumElements(const int &s)
	{
		(void)s;
		return 1;
	}


	static
	void x2native(int &i, const XPtrType data, unsigned int count)
	{
		i = static_cast<int>(*data);
		(void)count;
	}

	static
	void native2x(const int &s, XPtrType &data)
	{
		// We simply set the pointer to the PROPTYPE item as a flat
		// copy.
		data = (XPtrType)&s;
	}
};

template <>
//! property trait specialization for integers
class XPropTraits<XAtom>
{
public: // constants

	static const Atom x_type = XA_ATOM;
	static const unsigned long fixed_size = sizeof(Atom);
	typedef long* XPtrType;
	static const char format = 32;

public: // functions

	static
	int getNumElements(const int &s)
	{
		(void)s;
		return 1;
	}


	static
	void x2native(XAtom &a, const XPtrType data, unsigned int count)
	{
		a = static_cast<Atom>(*data);
		(void)count;
	}

	static
	void native2x(const XAtom &a, XPtrType &data)
	{
		// We simply set the pointer to the PROPTYPE item as a flat
		// copy.
		data = (XPtrType)a.getPtr();
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
	typedef const char* XPtrType;

public: // functions

	static
	void x2native(const char *&s, XPtrType data, unsigned int count)
	{
		(void)count;
		s = data;
	}

	static
	void native2x(const char * & s, XPtrType &data)
	{
		data = s;
	}

	static
	int getNumElements(const char* const s)
	{
		// strings in X are transferred without null terminator, the
		// library always provides a terminating null byte in
		// transferred data, however
		return s ? std::strlen(s) : 0;
	}
};

template <>
//! property type specialization for Window identifiers
class XPropTraits<Window>
{
public: // constants

	static const Atom x_type = XA_WINDOW;
	static const unsigned long fixed_size = sizeof(Window);
	static const char format = 32;
	typedef long* XPtrType;

public: // functions

	static
	void x2native(Window &w, XPtrType data, unsigned int count)
	{
		(void)count;
		w = static_cast<Window>(*data);
	}
};

template <>
//! property type specialization for utf8 strings
class XPropTraits<utf8_string>
{
public: // constants

	static XAtom x_type;
	static const unsigned long fixed_size = 0;
	static const char format = 8;
	typedef const char* XPtrType;

public: // functions

	static
	void init()
	{
		// this XLib property type atom is not available as a constant
		// in the Xlib headers but needs to be queried at runtime.
		x_type = StandardProps::instance().atom_ewmh_utf8_string;
	}

	static
	void x2native(utf8_string &s, XPtrType data, unsigned int count)
	{
		(void)count;
		s.str = data;
	}

	static
	void native2x(const utf8_string &s, XPtrType &data)
	{
		data = s.str.c_str();
	}

	static
	int getNumElements(const utf8_string &s)
	{
		// I suppose this is just for the X protocol the number of
		// bytes, not the number unicode characters
		return s.str.size();
	}
};


template <typename ELEM>
//! property type specialization for vectors of primitives
class XPropTraits< std::vector<ELEM> >
{
public: // constants

	static const Atom x_type = XPropTraits<ELEM>::x_type;
	static const unsigned long fixed_size = 0;
	static const char format = XPropTraits<ELEM>::format;
	typedef typename XPropTraits<ELEM>::XPtrType XPtrType;

public: // functions

	static
	void x2native(std::vector<ELEM> &v, XPtrType data, unsigned int count)
	{
		for( unsigned int e = 0; e < count; e++ )
		{
			v.push_back(data[e]);
		}
	}
};

template <>
//! property type specialization for vectors of primitives
class XPropTraits< std::vector<utf8_string> >
{
public: // constants

	static XAtom x_type;
	static const unsigned long fixed_size = 0;
	static const char format = XPropTraits<utf8_string>::format;
	typedef const char* XPtrType;

public: // functions

	static
	void init()
	{
		// correct order of runtime initialization is necessary in
		// Xwmfs::early_init().
		x_type = XPropTraits<utf8_string>::x_type;
	}

	static
	void x2native(std::vector<utf8_string> &v, XPtrType data, unsigned int count)
	{
		// we get a char* sequence of zero-terminated strings here
		v.clear();

		while( count != 0 )
		{
			v.push_back(utf8_string(data));
			const auto consumed = v.back().length() + 1;
			count -= consumed;
			data += consumed;
		}
	}
};

template <>
//! property type specialization for arrays of Window identifiers
class XPropTraits< std::vector<int> >
{
public: // constants

	static const Atom x_type = XA_CARDINAL;
	static const unsigned long fixed_size = 0;
	static const char format = 32;
	typedef long* XPtrType;

public: // functions

	static
	void x2native(std::vector<int> &v, XPtrType data, unsigned int count)
	{
		v.clear();

		for( unsigned int e = 0; e < count; e++ )
		{
			v.push_back(data[e]);
		}
	}
};

template <>
//! property type specialization for arrays of XAtoms
class XPropTraits< std::vector<XAtom> >
{
public: // constants

	static const Atom x_type = XA_ATOM;
	static const unsigned long fixed_size = 0;
	static const char format = 32;
	typedef long* XPtrType;

public: // functions

	static
	void x2native(std::vector<XAtom> &v, XPtrType data, unsigned int count)
	{
		for( unsigned int e = 0; e < count; e++ )
		{
			v.push_back(XAtom(data[e]));
		}
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
	typedef typename XPropTraits<PROPTYPE>::XPtrType XPtrType;

public: // functions

	//! construct an empty/default property value
	Property() : m_native() { }

	//! forbid copying to avoid trouble with memory mgm.
	Property(const Property&) = delete;

	//! construct a property holding the value from \c p
	Property(const PROPTYPE &p) : m_native()
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
			xwmfs_throw(Exception("No valid property stored")); 
		}

		return m_native;
	}

	/**
	 * \brief
	 * 	Retrieves a pointer to the raw data associated with the
	 * 	property
	 **/
	typename Traits::XPtrType getRawData() const { return m_data; }

	//! returns whether valid property data is set
	bool valid() const { return m_data != nullptr; }

	/**
	 * \brief
	 * 	Assigns the given property value from \c p
	 **/
	Property& operator=(const PROPTYPE &p)
	{
		checkDelete();

		m_native = p;
		Traits::native2x( m_native, m_data );

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

		if( Traits::fixed_size && size > Traits::fixed_size)
		{
			xwmfs_throw(Exception("size is larger than fixed_size")); 
		}

		m_data_is_from_x = true;
		m_data = reinterpret_cast<XPtrType>(data);

		Traits::x2native(
			m_native,
			m_data,
			size / (Traits::format / 8)
		);
	}

	//! Retrieves the associated XAtom type from the traits of PROPTYPE
	static Atom getXType() { return Traits::x_type; }

	/**
	 * \brief
	 * 	If the current Property instance contains data allocated by
	 * 	Xlib then it is deleted
	 **/
	void checkDelete()
	{
		// frees the m_data ptr if it comes from xlib
		if( m_data_is_from_x )
		{
			// note: XFree returns strange codes ...

			/*const int res = */XFree( (void*)m_data);
			m_data_is_from_x = false;
		}
	}

private: // data

	//! The native property type
	PROPTYPE m_native;
	//! determines whether the pointer m_data is from Xlib and needs to be
	//! freed
	bool m_data_is_from_x = false;
	//! A pointer to m_native that can be fed to Xlib
	typename Traits::XPtrType m_data = nullptr;
};

} // end ns

#endif // inc. guard

