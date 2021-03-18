#include "main/StdLogger.hxx"

#include <iostream>

namespace xwmfs
{

StdLogger::StdLogger()
{
	setStreams(std::cout, std::cout, std::cout, std::cerr);
}

StdLogger::~StdLogger()
{
	// make sure any outstanding data is displayed by now
	std::cout << std::flush;
}

} // end ns
