#ifndef XWMFS_HELPER_HXX
#define XWMFS_HELPER_HXX

// C++
#include <string>
#include <cstring>

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

//! for comparison in maps with char pointers as keys
struct compare_cstring
{
	bool operator()(const char *a, const char *b) const
	{ return std::strcmp(a, b) < 0; }
};

/**
 * \brief
 * 	Returns whether prefix is a prefix of s
 **/
inline bool isprefix(const std::string &s, const std::string &prefix)
{
	return s.substr(0, prefix.length()) == prefix;
}

} // end ns

#endif // inc. guard

