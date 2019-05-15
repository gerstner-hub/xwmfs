#ifndef XWMFS_EXCEPTION_HXX
#define XWMFS_EXCEPTION_HXX

#include <stdint.h>

#include <vector>
#include <string>

#define XWMFS_SRC_LOCATION xwmfs::Exception::SourceLocation(__FILE__, __LINE__, __FUNCTION__)
//! helper macro to throw an arbitrary exception type while implicitly adding
//! the current source code location
#define xwmfs_throw(ex) (ex).setSourceLoc(xwmfs::Exception::SourceLocation(__FILE__, __LINE__, __FUNCTION__)).throwIt();
#define XWMFS_EXCEPTION_IMPL public: [[ noreturn ]] void throwIt() const override { throw *this; }

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
		constexpr SourceLocation(
			const char *p_file = nullptr,
			const int p_line = -1,
			const char *p_func = nullptr
		) :
			file(p_file), line(p_line), func(p_func) { }

		const char* getFile() const { return file; }
		int getLine() const { return line; }
		const char* getFunction() const { return func; }

	protected: // data

		const char *file;
		int line;
		const char *func;
	};

public: // functions

	Exception(const std::string &err) : m_error(err) {}

	std::string what(const uint32_t level=0) const;

	void addError(const xwmfs::Exception &ex)
	{
		m_pre_errors.push_back(ex);
	}

	Exception& setSourceLoc(const SourceLocation &location)
	{
		m_location = location;
		return *this;
	}

	/**
	 * \brief
	 * 	Helper function to throw the most derived type of this object
	 **/
	[[ noreturn ]] virtual void throwIt() const { throw *this; }

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

