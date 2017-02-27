#ifndef XWMFS_X11EXCEPTION_HXX
#define XWMFS_X11EXCEPTION_HXX

#include <stdint.h>

// main xlib header, provides Display declaration
#include "X11/Xlib.h"

#include "common/Exception.hxx"

namespace xwmfs
{

/**
 * \brief
 * 	Exception type for X11 errors
 * \details
 * 	The type additionally takes an X int return code for which an error
 * 	description is produced within the class.
 **/
class X11Exception :
	public Exception
{
public: // functions
	X11Exception(Display *dis, const int errcode) :
		Exception("X11 operation failed: \"")
	{
		char errtext[128];
		(void)XGetErrorText(dis, errcode, errtext, 128);
		m_error += errtext;
		m_error += '"';
	}

	XWMFS_EXCEPTION_IMPL;
};

} // end ns

#endif // inc. guard

