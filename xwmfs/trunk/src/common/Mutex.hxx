#ifndef XWMFS_MUTEX_HXX
#define XWMFS_MUTEX_HXX

#include <pthread.h>
#include <assert.h>

#include "common/SystemException.hxx"

namespace xwmfs
{

// fwd. decl.
class Condition;

/**
 * \brief
 * 	A class to represent a pthread mutex
 * \details
 * 	Only the most basic operations are implemented by now. For more
 * 	details refer to the POSIX man pages.
 **/
class Mutex
{
public: // functions

	/**
	 * \brief
	 *	The only supported mutex type for the moment is non-recursive
	 *	as others aren't needed
	 * \details
	 *	In NDEBUG is not set then additional error checks are in
	 *	effect that allow detection of deadlocks etc.
	 **/
	Mutex()
	{
		::pthread_mutexattr_t* attr = NULL;
#ifndef NDEBUG
		::pthread_mutexattr_t debug_attr;
		const int attr_init_res = ::pthread_mutexattr_init(&debug_attr);

		if( attr_init_res )
		{
			throw xwmfs::SystemException( XWMFS_SRC_LOCATION,
				"Error creating debug mutex attribute" );
		}

		const int settype_res = ::pthread_mutexattr_settype(
			&debug_attr,
			PTHREAD_MUTEX_ERRORCHECK);

		if( settype_res )
		{
			throw xwmfs::SystemException( XWMFS_SRC_LOCATION,
				"Error setting debug mutex type" );
		}

		attr = &debug_attr;
#endif

		const int mutex_init_res = ::pthread_mutex_init(
			&m_pmutex,
			attr);
		if( mutex_init_res )
		{
			throw xwmfs::SystemException( XWMFS_SRC_LOCATION,
				"Error creating mutex" );
		}

#ifndef NDEBUG
		const int attr_destr_res = ::pthread_mutexattr_destroy(
			&debug_attr);
		assert( ! attr_destr_res );
#endif
	}

	~Mutex()
	{
		const int destroy_res = ::pthread_mutex_destroy(&m_pmutex);

		assert( !destroy_res );
	}

	void lock() const
	{
		const int lock_res = ::pthread_mutex_lock(&m_pmutex);

		if( lock_res )
		{
			throw xwmfs::SystemException( XWMFS_SRC_LOCATION,
				"Error locking mutex" );
		}
	}

	void unlock() const
	{
		const int unlock_res = ::pthread_mutex_unlock(&m_pmutex);

		if( unlock_res )
		{
			throw xwmfs::SystemException( XWMFS_SRC_LOCATION,
				"Error unlocking mutex" );
		}
	}

protected: // functions

	// protected copy ctor. and assignment op. to avoid copies
	Mutex(const Mutex&);
	Mutex& operator=(const Mutex&);

private: // data

	// make that mutable to make const lock/unlock semantics possible
	mutable pthread_mutex_t m_pmutex;

	// Condition needs access to our handle
	friend class Condition;
};

} // end ns

#endif // inc. guard

