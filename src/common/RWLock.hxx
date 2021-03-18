#ifndef XWMFS_RWLOCK_HXX
#define XWMFS_RWLOCK_HXX

#include <pthread.h>
#include <assert.h>

#include "common/SystemException.hxx"

namespace xwmfs
{

/**
 * \brief
 * 	A class to represent a pthread read-write lock
 * \details
 *	A read-write lock can be locked in parallel for reading but only by
 *	one thread for writing. This is helpful if you got data that is
 *	updated rarely but read often.
 *
 * 	Only the most basic operations are provided by now. For more
 * 	information please refer to the POSIX man pages.
 **/
class RWLock
{
	// forbid copy-assignment
	RWLock(const RWLock&) = delete;
	RWLock& operator=(const RWLock&) = delete;
public: // functions

	RWLock()
	{
		const int rwlock_init_res = ::pthread_rwlock_init(
			&m_prwlock, nullptr
		);

		if( rwlock_init_res )
		{
			xwmfs_throw(
				xwmfs::SystemException("Error creating rwlock" )
			);
		}
	}

	~RWLock()
	{
		const int destroy_res = ::pthread_rwlock_destroy(&m_prwlock);

		assert( !destroy_res );
	}

	void readlock() const
	{
		const int lock_res = ::pthread_rwlock_rdlock(&m_prwlock);

		if( lock_res )
		{
			xwmfs_throw(
				xwmfs::SystemException("Error read-locking rwlock")
			);
		}
	}

	void writelock() const
	{
		const int lock_res = ::pthread_rwlock_wrlock(&m_prwlock);

		if( lock_res )
		{
			xwmfs_throw(
				xwmfs::SystemException("Error write-locking rwlock")
			);
		}
	}

	void unlock() const
	{
		const int unlock_res = ::pthread_rwlock_unlock(&m_prwlock);

		if( unlock_res )
		{
			xwmfs_throw(
				xwmfs::SystemException("Error unlocking mutex" )
			);
		}
	}

protected: // data

	// make that mutable to make const lock/unlock semantics possible
	mutable pthread_rwlock_t m_prwlock;
};

/**
 * \brief
 * 	A lock-guard object that locks an RWLock for reading until it is
 * 	destroyed
 **/
class ReadLockGuard
{
public: // functions

	ReadLockGuard(const RWLock &rwl) :
		m_rwl(rwl)
	{
		rwl.readlock();
	}

	~ReadLockGuard()
	{
		m_rwl.unlock();
	}
private: // data
	const RWLock &m_rwl;
};

/**
 * \brief
 * 	A lock-guard object that locks an RWLock for writing until it is
 * 	destroyed
 **/
class WriteLockGuard
{
public: // functions

	WriteLockGuard(const RWLock &rwl) :
		m_rwl(rwl)
	{
		rwl.writelock();
	}

	~WriteLockGuard()
	{
		m_rwl.unlock();
	}

private: // data
	const RWLock &m_rwl;
};

} // end ns

#endif // inc. guard
