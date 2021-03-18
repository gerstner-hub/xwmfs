// xwmfs
#include "x11/XWindowAttrs.hxx"

namespace xwmfs
{

bool XWindowAttrs::isMapped() const
{
	return map_state != IsUnmapped;
}

} // end ns
