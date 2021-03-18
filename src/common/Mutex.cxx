#include "common/Mutex.hxx"

namespace xwmfs
{

#ifndef NDEBUG
static const bool DEBUG_MUTEX = true;
#else
static const bool DEBUG_MUTEX = false;
#endif

Mutex::Mutex()
{
	::pthread_mutexattr_t* attr = nullptr;

	if( DEBUG_MUTEX )
	{
		::pthread_mutexattr_t debug_attr;
		if( ::pthread_mutexattr_init(&debug_attr) != 0 )
		{
			xwmfs_throw(
				xwmfs::SystemException("Error creating debug mutex attribute")
			);
		}

		if( ::pthread_mutexattr_settype(
			&debug_attr, PTHREAD_MUTEX_ERRORCHECK
			) != 0 )
		{
			xwmfs_throw(
				xwmfs::SystemException("Error setting debug mutex type")
			);
		}


		attr = &debug_attr;
	}

	if( ::pthread_mutex_init(&m_pmutex, attr) != 0 )
	{
		xwmfs_throw(
			xwmfs::SystemException("Error creating mutex" )
		);
	}

	if( DEBUG_MUTEX )
	{
		const int attr_destr_res = ::pthread_mutexattr_destroy(
			attr
		);
		assert( ! attr_destr_res );
	}
}

} // end ns
