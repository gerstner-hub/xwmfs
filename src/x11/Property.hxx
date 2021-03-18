#ifndef XWMFS_PROPERTY
#define XWMFS_PROPERTY

// xwmfs
#include "x11/PropertyTraits.hxx"

namespace xwmfs
{

/**
 * \brief
 *	X11 property representation
 * \details
 * 	Based on the PropertyTraits definitions above this class allows to
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
	typedef PropertyTraits<PROPTYPE> Traits;
	//! The correct pointer type for our property type
	typedef typename PropertyTraits<PROPTYPE>::XPtrType XPtrType;

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
