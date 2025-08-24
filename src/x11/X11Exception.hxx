#ifndef XWMFS_X11EXCEPTION_HXX
#define XWMFS_X11EXCEPTION_HXX

#include <stdint.h>

// main xlib header, provides Display declaration
#include "X11/Xlib.h"

// xwmfs
#include "main/Exception.hxx"

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
	public Exception {
public: // functions
	X11Exception(Display *dis, const int errcode,
			const cosmos::SourceLocation &src_loc = cosmos::SourceLocation::current()) :
		Exception{"X11 operation failed: \"", src_loc},
		m_dis{dis}, m_errcode{errcode} {
	}

	void generateMsg() const override {
		char errtext[128];
		(void)XGetErrorText(m_dis, m_errcode, errtext, sizeof(errtext));
		m_msg += errtext;
		m_msg += '"';
	}
protected:
	Display *m_dis;
	int m_errcode;
};

} // end ns

#endif // inc. guard
