#ifndef XWMFS_ILOGGER_HXX
#define XWMFS_ILOGGER_HXX

// C++
#include <iosfwd>
#include <sstream>

namespace xwmfs
{

/**
 * \brief
 * 	Abstract interface for a logging facility
 * \details
 * 	Applications can use this interface to log data to arbitrary places.
 * 	You need to derive from this interface and decide what places these
 * 	are.
 *
 * 	The base class writes data to std::ostream instances. So
 * 	implementation of this class need to provide some instance of
 * 	std::ostream for writing to.
 *
 * 	This base class additionally provides means to write colored text and
 * 	detect whether an ostream is connected to a terminal.
 **/
class ILogger
{
public: // types

	/**
	 * \brief
	 * 	Color constants with correct values for ANSI escape sequences
	 **/
	enum class Color : size_t
	{
		BLACK = 0,
		RED,
		GREEN,
		YELLOW,
		BLUE,
		MAGENTA,
		CYAN,
		WHITE,
		NONE
	};

public: // functions

	virtual ~ILogger() {};

	std::ostream& error()
	{
		return getStream(
			*m_err, "Error: ", Color::RED,
			m_err_enabled, m_err_is_tty
		);
	}

	std::ostream& warn()
	{
		return getStream(
			*m_warn, "Warning: ", Color::YELLOW,
			m_warn_enabled, m_warn_is_tty
		);
	}

	std::ostream& info()
	{
		return getStream(
			*m_info, "Info: ", Color::NONE,
			m_info_enabled, m_info_is_tty
		);
	}

	std::ostream& debug()
	{
		return getStream(
			*m_debug, "Debug: ", Color::CYAN,
			m_debug_enabled, m_debug_is_tty
		);
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

protected: // functions

	std::ostream& getStream(
		std::ostream &s,
		const char *prefix,
		const Color &color,
		const bool &enabled,
		const bool &tty)
	{
		auto &out = enabled ? s : getNoopStream();

		if(tty)
			startColor(out, color);

		out << prefix;

		if(tty)
			finishColor(out);

		return out;
	}

	std::ostream& getNoopStream()
	{
		// a real noop implementation of an ostream might still be
		// cheaper than this, but at least it doesn't involve system
		// calls
		//
		// seek the start of the stringstream, to avoid the buffer
		// growing too much
		m_null.seekp( m_null.beg );
		return m_null;
	}

	std::ostream& startColor(std::ostream &o, const Color &c);
	std::ostream& finishColor(std::ostream &o);

	/**
	 * \brief
	 * 	Returns whether the given ostream is associated with a
	 * 	terminal or not
	 **/
	static bool isTTY(const std::ostream &o);

	void setStreams(
		std::ostream &debug,
		std::ostream &info,
		std::ostream &warn,
		std::ostream &error
	);

protected: // data

	std::ostream *m_err = nullptr;
	std::ostream *m_warn = nullptr;
	std::ostream *m_info = nullptr;
	std::ostream *m_debug = nullptr;

	//! a noop stream object to write to if a channel is disabled
	std::stringstream m_null;

	bool m_err_enabled = true;
	bool m_warn_enabled = true;
	bool m_info_enabled = true;
	bool m_debug_enabled = false;

	bool m_err_is_tty = false;
	bool m_warn_is_tty = false;
	bool m_info_is_tty = false;
	bool m_debug_is_tty = false;
};

} // end ns

#endif // inc. guard
