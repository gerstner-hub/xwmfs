// xwmfs
#include "common/ILogger.hxx"

// C++
#include <iostream>

// POSIX
#include <unistd.h> // STD*_FILENO, isatty

namespace xwmfs
{

/*
 * constants for ANSI color escape sequences
 */

// the ASCII escape character for ANSI control sequences
const char COLOR_ESCAPE = '\033';
// ANSI escape finish character
const char COLOR_FINISH = 'm';
const char COLOR_RESET[] = "[0";
const char COLOR_FG[] = "[3";
const char COLOR_BRIGHT[] = "[1";
const char COLOR_NORMAL[] = "[22";

std::ostream& ILogger::startColor(std::ostream &o, const Color &c)
{
	if( c == Color::NONE )
		return o;

	o << COLOR_ESCAPE << COLOR_FG << char('0' + (size_t)c) << COLOR_FINISH;
	return o;
}

std::ostream& ILogger::finishColor(std::ostream &o)
{
	o << COLOR_ESCAPE << COLOR_RESET << COLOR_FINISH;
	return o;
}

bool ILogger::isTTY(const std::ostream &o)
{
	/*
	 * there's no elegant, portable way to get the file descriptor from an
	 * ostream thus we have to use some heuristics ...
	 */

	int fd_to_check = -1;

	if( &o == &std::cout )
	{
		fd_to_check = STDOUT_FILENO;
	}
	else if( &o == &std::cerr )
	{
		fd_to_check = STDERR_FILENO;
	}

	if( fd_to_check == -1 )
		return false;

	return ::isatty(fd_to_check) == 1;
}

void ILogger::setStreams(
	std::ostream &debug,
	std::ostream &info,
	std::ostream &warn,
	std::ostream &err
)
{
	m_err = &err;
	m_warn = &warn;
	m_info = &info;
	m_debug = &debug;

	m_err_is_tty = isTTY(err);
	m_warn_is_tty = isTTY(warn);
	m_info_is_tty = isTTY(info);
	m_debug_is_tty = isTTY(debug);
}

} // end ns

