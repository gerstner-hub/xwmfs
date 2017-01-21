// C++
#include <sstream>

// xwmfs
#include "common/Exception.hxx"


namespace xwmfs
{

std::string Exception::what(const uint32_t level) const 
{
	std::stringstream ret;

	indent(level, ret);

	ret << m_error << " @ "
		<< m_location.getFile() << ":" << m_location.getLine()
		<< " in " << m_location.getFunction() << "()" << "\n";

	for( const auto &pre_error: m_pre_errors )
	{
		ret << pre_error.what(level+1);
	}

	return ret.str();
}
	
void Exception::indent(const uint32_t level, std::ostream &o) const
{
	o << std::string(level, '\t') << level << "): ";
}

} // end ns

