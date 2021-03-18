#ifndef XWMFS_CONDITION_HXX
#define XWMFS_CONDITION_HXX

#include <pthread.h>

#include "common/SystemException.hxx"
#include "common/Mutex.hxx"

namespace xwmfs
{

/**
 * \brief
 * 	A class to represent a pthread condition
 * \details
 * 	The current implementation only provides the most basic condition
 * 	operations. Refer to the POSIX man pages for more information.
 **/
class Condition
{
	// forbid copy-assignment
	Condition(const Condition&) = delete;
	Condition& operator=(const Condition&) = delete;

public: // functions

	/**
	 * \brief
	 * 	Create a condition coupled with \c lock
	 * \details
	 *	The given lock will be associated with the Condition for the
	 *	complete lifetime of the object. You need to make sure that
	 *	\c lock is never destroyed before the associated Condition
	 *	object is destroyed.
	 **/
	Condition(Mutex &lock) :
		m_lock(lock)
	{
		if( ::pthread_cond_init(&m_pcond, nullptr) != 0 )
		{
			xwmfs_throw(
				xwmfs::SystemException("Error creating condition" )
			);
		}
	}

	~Condition()
	{
		const int destroy_res = ::pthread_cond_destroy(&m_pcond);

		assert( ! destroy_res );
	}

	//! The associated lock must already be locked at entry
	void wait()
	{
		if( ::pthread_cond_wait(&m_pcond, &(m_lock.m_pmutex)) != 0 )
		{
			xwmfs_throw(
				xwmfs::SystemException("Error waiting on condition" )
			);
		}
	}

	void signal()
	{
		if( ::pthread_cond_signal(&m_pcond) != 0 )
		{
			xwmfs_throw(
				xwmfs::SystemException("Error signaling condition" )
			);
		}
	}

	void broadcast()
	{
		if( ::pthread_cond_broadcast(&m_pcond) != 0 )
		{
			xwmfs_throw(
				xwmfs::SystemException("Error broadcasting condition" )
			);
		}
	}

	Mutex& getMutex() { return m_lock; }

protected: // data

	pthread_cond_t m_pcond;
	Mutex &m_lock;
};

} // end ns

#endif // inc. guard
