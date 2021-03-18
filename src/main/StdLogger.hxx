#ifndef XWMFS_STDLOGGER_HXX
#define XWMFS_STDLOGGER_HXX

#include "common/ILogger.hxx"

namespace xwmfs
{

/*
 * \brief
 * 	A simple standard logger that logs to cout/cerr
 * \details
 * 	Except the error streams all logged data goes to std::cout. The error
 * 	stream, of course, goes to std::cerr.
 */
class StdLogger :
	public ILogger
{
public:
	~StdLogger();

	static StdLogger& getInstance()
	{
		static StdLogger logger;

		return logger;
	}

protected:
	StdLogger();
};

} // end ns

#endif // inc. guard
