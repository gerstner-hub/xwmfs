#include <stdlib.h>

#include "common/Mutex.hxx"

namespace xwmfs
{

namespace {

::pthread_mutexattr_t debug_attr;
bool debug_attr_initialized = false;

#ifndef NDEBUG
constexpr bool DEBUG_MUTEX = true;
#else
constexpr bool DEBUG_MUTEX = false;
#endif

void cleanupDebug() {
	if (!DEBUG_MUTEX)
		return;
	else if (!debug_attr_initialized)
		return;

	const int attr_destr_res = ::pthread_mutexattr_destroy(
		&debug_attr
	);
	assert( ! attr_destr_res );
	debug_attr_initialized = false;
}

void checkInitDebug() {
	if (!DEBUG_MUTEX)
		return;
	else if (debug_attr_initialized)
		return;

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

	atexit(cleanupDebug);
	debug_attr_initialized = true;
}

} // end anon ns

Mutex::Mutex()
{
	checkInitDebug();
	::pthread_mutexattr_t* attr = DEBUG_MUTEX ? &debug_attr : nullptr;


	if( ::pthread_mutex_init(&m_pmutex, attr) != 0 )
	{
		xwmfs_throw(
			xwmfs::SystemException("Error creating mutex" )
		);
	}
}

} // end ns
