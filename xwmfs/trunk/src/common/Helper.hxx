#ifndef XWMFS_HELPER_HXX
#define XWMFS_HELPER_HXX

// C++
#include <string>

namespace xwmfs
{

//! returns an all lower case version of \c s
std::string tolower(const std::string &s);
void strip(std::string &s);
inline std::string stripped(const std::string &s)
{
	auto ret(s);
	strip(ret);
	return ret;
}

} // end ns

#endif // inc. guard

