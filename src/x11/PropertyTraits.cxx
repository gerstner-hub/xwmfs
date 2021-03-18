// own header
#include "x11/PropertyTraits.hxx"

namespace xwmfs
{

XAtom PropertyTraits<utf8_string>::x_type = XAtom(0);
XAtom PropertyTraits<std::vector<utf8_string>>::x_type = XAtom(0);

} // end ns
