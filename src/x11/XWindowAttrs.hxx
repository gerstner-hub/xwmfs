#ifndef XWMFS_ATTRIBUTES_HXX
#define XWMFS_ATTRIBUTES_HXX

// X
#include "X11/Xlib.h"

namespace xwmfs
{

class XWindowAttrs :
	public XWindowAttributes
{
public: // functions

	bool isMapped() const;
};

} // end ns

#endif // inc. guard
