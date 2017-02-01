// own header
#include "x11/property.hxx"

namespace xwmfs
{

XAtom XPropTraits<utf8_string>::x_type = XAtom(0);

// make sure the address of a utf8_string instance is the same as the one for
// its only member
static_assert( sizeof(utf8_string) == sizeof(char*), "bad utf8_string alignment" );

} // end ns

