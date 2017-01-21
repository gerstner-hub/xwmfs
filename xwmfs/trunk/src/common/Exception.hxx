#ifndef XWMFS_EXCEPTION_HXX
#define XWMFS_EXCEPTION_HXX

#include <stdint.h>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#define XWMFS_SRC_LOCATION xwmfs::Exception::SourceLocation(__FILE__, __LINE__, __FUNCTION__)

namespace xwmfs
{

/**
 * \brief
 * 	Basic exception class
 * \details
 * 	An Exception provides information about the code file and line
 * 	where it was first thrown as well as a dynamically allocated
 * 	string containing a description about what the problem is.
 *
 * 	An Exception additionally can contain other exceptions to define
 * 	an error context i.e. errors that are related to each other.
 * 	E.g. the root cause of an error and follow-up errors that
 * 	resulted from it.
 **/
class Exception
{
public: // types

	/**
	 * \brief
	 * 	Merged source file, source line and function information
	 **/
	struct SourceLocation
	{
		SourceLocation(
			const char *p_file, const int p_line, const char *p_func
		) :
			file(p_file), line(p_line), func(p_func) { }

		SourceLocation(const SourceLocation &other) :
			file(other.file),
			line(other.line),
			func(other.func)
		{ }

		const char* getFile() const { return file; }
		int getLine() const { return line; }
		const char* getFunction() const { return func; }

	protected: // data

		const char *file;
		int line;
		const char *func;
	};

public: // functions

	Exception(const SourceLocation sl, const std::string &err) :
		m_location(sl),
		m_error(err)
	{}

	std::string what(const uint32_t level=0) const;

	void addError(const xwmfs::Exception &ex)
	{
		m_pre_errors.push_back(ex);
	}

protected: // data

	//! file and line of Exception origin
	SourceLocation m_location;
	//! error description
	std::string m_error;

private: // functions

	/**
	 * \brief
	 * 	Creates the given indentation \c level on the given ostream \c
	 * 	o
	 **/
	void indent(const uint32_t level, std::ostream &o) const;

private: // data

	// errors that have been detected before this one
	std::vector<xwmfs::Exception> m_pre_errors;
};

} // end ns

//! \brief
//! stream output operator that allows to directly print an exception
//! onto an ostream
inline std::ostream& operator<<(
	std::ostream &o,
	const xwmfs::Exception &e)
{
	o << e.what();

	return o;
}

#endif // inc. guard

