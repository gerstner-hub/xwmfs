#include "main/StdLogger.hxx"

#include <iostream>

namespace xwmfs
{

StdLogger::StdLogger()
{
	m_debug = static_cast<std::ostream*>(&std::cout);
	m_info = static_cast<std::ostream*>(&std::cout);
	m_warn = static_cast<std::ostream*>(&std::cout);
	m_err = static_cast<std::ostream*>(&std::cerr);
}

StdLogger::~StdLogger()
{
	// make sure any outstanding data is displayed by now
	std::cout << std::flush;
}

} // end ns

