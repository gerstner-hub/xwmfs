#ifndef XWMFS_SYSTEMEXCEPTION_HXX
#define XWMFS_SYSTEMEXCEPTION_HXX

#include <errno.h>
#include <string.h>

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
	SystemException(const std::string &err);

private: // members

	//! The errno that was seen during construction time
	int m_errno;

	XWMFS_EXCEPTION_IMPL;
};

} // end ns

#endif // inc. guard

