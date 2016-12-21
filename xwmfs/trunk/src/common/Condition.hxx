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
		const int init_res = ::pthread_cond_init(&m_pcond, NULL);

		assert( ! init_res );
	}

	~Condition()
	{
		const int destroy_res = ::pthread_cond_destroy(&m_pcond);

		assert( ! destroy_res );
	}

	//! The associated lock must already be locked at entry
	void wait()
	{
		const int wait_res = ::pthread_cond_wait(&m_pcond, &(m_lock.m_pmutex));

		assert( ! wait_res );
	}

	void signal()
	{
		const int sig_res = ::pthread_cond_signal(&m_pcond);

		assert( ! sig_res );
	}

protected: // functions

	// protected copy ctor. and assignment op. to avoid copies
	Condition(const Condition&);
	Condition& operator=(const Condition&);

private: // data
	pthread_cond_t m_pcond;
	Mutex &m_lock;
};

} // end ns

#endif // inc. guard

