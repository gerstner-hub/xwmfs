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

void strip(std::string &s)
{
	while( !s.empty() && std::isspace(s[0]) )
		s.erase( s.begin() );

	while( !s.empty() && std::isspace(s[s.length()-1]) )
		s.pop_back();
}

} // end ns

