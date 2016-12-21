#ifndef XWMFS_ILOGGER_HXX
#define XWMFS_ILOGGER_HXX

#include <ostream>
#include <fstream>

namespace xwmfs
{

/**
 * \brief
 * 	Abstract interface for a logging facility
 * \details
 * 	Applications can use this interface to log data to arbitrary places.
 * 	You need to dervice from this interface and decide what places these
 * 	are.
 *
 * 	The base class writes data to std::ostream instances. So
 * 	implementation of this class need to provide some instance of
 * 	std::ostream for writing to.
 **/
class ILogger
{
public: // functions

	ILogger() :
		m_err(),
		m_warn(),
		m_info(),
		m_debug(),
		m_err_enabled(true),
		m_warn_enabled(true),
		m_info_enabled(true),
		m_debug_enabled(false)
	{
		// XXX: This null device handling is somewhat expensive for
		// disabled channels as we're still passing data through our
		// program just to discard it.
		m_null.open( "/dev/null" );
	}

	virtual ~ILogger() {};

	std::ostream& error()
	{
		if( !m_err_enabled )
			return m_null;
		*m_err << "Error: ";
		return *m_err;
	}

	std::ostream& warn()
	{
		if( !m_warn_enabled )
			return m_null;

		*m_warn << "Warning: ";
		return *m_warn;
	}

	std::ostream& info()
	{
		if( !m_info_enabled )
			return m_null;

		*m_info << "Info: ";
		return *m_info;
	}

	std::ostream& debug()
	{
		if( !m_debug_enabled )
			return m_null;

		*m_debug << "Debug: ";
		return *m_debug;
	}

	void setChannels(
		const bool error,
		const bool warning,
		const bool info,
		const bool debug
	)
	{
		m_err_enabled = error;
		m_warn_enabled = warning;
		m_info_enabled = info;
		m_debug_enabled = debug;
	}

protected: // data

	std::ostream *m_err;
	std::ostream *m_warn;
	std::ostream *m_info;
	std::ostream *m_debug;

	//! a null stream object to write to if a channel is disabled
	std::ofstream m_null;

	bool m_err_enabled;
	bool m_warn_enabled;
	bool m_info_enabled;
	bool m_debug_enabled;
};

} // end ns

#endif // inc. guard

