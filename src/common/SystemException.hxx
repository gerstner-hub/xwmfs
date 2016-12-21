#ifndef XWMFS_SYSTEMEXCEPTION_HXX
#define XWMFS_SYSTEMEXCEPTION_HXX

#include <errno.h>
#include <string.h>
#include <sstream>

#include "common/Exception.hxx"

namespace xwmfs
{

/**
 * \brief
 * 	Specialized exception for system API / call errors reported via errno
 * \details
 * 	This exception type adds system error information to the exception
 * 	text.
 **/
class SystemException :
	public xwmfs::Exception
{
public: // functions

	/**
	 * \brief
	 * 	The string provided in \c err will be appended with the
	 * 	errno description of the current thread
	 **/
	SystemException(const SourceLocation sl, const std::string &err) :
		Exception(sl, err)
	
	{
		m_errno = errno;

		char msg[256];

		char *m = ::strerror_r(m_errno, msg, 256);
		std::stringstream str_errno;
		str_errno << m_errno;

		m_error.append(" (\"");
		m_error.append(m);
		m_error.append("\", errno = ");
		m_error.append(str_errno.str());
		m_error.append(")");
	}

private: // members

	//! The errno that was seen during construction time
	int m_errno;
};

} // end ns

#endif // inc. guard


