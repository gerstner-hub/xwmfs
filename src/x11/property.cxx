// own header
#include "x11/property.hxx"

namespace xwmfs
{

XAtom XPropTraits<utf8_string>::x_type = XAtom(0);
XAtom XPropTraits<std::vector<utf8_string>>::x_type = XAtom(0);

} // end ns

