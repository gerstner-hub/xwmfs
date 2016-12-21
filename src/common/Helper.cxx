// xwmfs
#include "common/Helper.hxx"

// C++
#include <algorithm>
#include <cctype> 

namespace xwmfs
{

std::string tolower(const std::string &s)
{
	std::string ret;
	ret.resize(s.size());
	// put it all to lower case
	std::transform(
		s.begin(), s.end(),
		ret.begin(),
		::tolower
	);

	return ret;
}

} // end ns

