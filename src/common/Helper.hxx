#ifndef XWMFS_HELPER_HXX
#define XWMFS_HELPER_HXX

// C++
#include <string>

namespace xwmfs
{

//! returns an all lower case version of \c s
std::string tolower(const std::string &s);

//! strips leading and trailing whitespace from the given string
void strip(std::string &s);

//! returns a version of the given string with leading and trailing whitespace
//! stripped away
inline std::string stripped(const std::string &s)
{
	auto ret(s);
	strip(ret);
	return ret;
}

} // end ns

#endif // inc. guard

