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

	// disallow copy/assignment
	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;

	/**
	 * \brief
	 *	The only supported mutex type for the moment is non-recursive
	 *	as others aren't needed
	 * \details
	 *	In NDEBUG is not set then additional error checks are in
	 *	effect that allow detection of deadlocks etc.
	 **/
	Mutex();

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
			xwmfs_throw(
				xwmfs::SystemException("Error locking mutex")
			);
		}
	}

	void unlock() const
	{
		const int unlock_res = ::pthread_mutex_unlock(&m_pmutex);

		if( unlock_res )
		{
			xwmfs_throw(
				xwmfs::SystemException("Error unlocking mutex" )
			);
		}
	}

protected: // data

	// make that mutable to make const lock/unlock semantics possible
	mutable pthread_mutex_t m_pmutex;

	// Condition needs access to our handle
	friend class Condition;
};

/**
 * \brief
 * 	A mutex guard object that locks a Mutex until it's destroyed
 **/
class MutexGuard
{
public: // functions

	MutexGuard(const Mutex &m) :
		m_mutex(m)
	{
		m_mutex.lock();
	}

	~MutexGuard()
	{
		m_mutex.unlock();
	}

private: // data

	const Mutex &m_mutex;
};

/**
 * \brief
 * 	A reversed mutex guard object that unlocks a Mutex until it's
 * 	destroyed
 **/
class MutexReverseGuard
{
public: // functions

	MutexReverseGuard(const Mutex &m) :
		m_mutex(m)
	{
		m_mutex.unlock();
	}

	~MutexReverseGuard()
	{
		m_mutex.lock();
	}

private: // data

	const Mutex &m_mutex;
};

} // end ns

#endif // inc. guard

