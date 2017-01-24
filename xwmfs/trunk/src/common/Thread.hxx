#ifndef XWMFS_THREAD_HXX
#define XWMFS_THREAD_HXX

#include <pthread.h>

#include "common/SystemException.hxx"
#include "common/Mutex.hxx"
#include "common/Condition.hxx"
#include "common/IThreadEntry.hxx"

namespace xwmfs
{

/**
 * \brief
 * 	A class representing a POSIX thread and its lifecycle
 * \details
 * 	There's a simple lifecycle modelled for the thread. Refer to
 * 	Thread::State for more details.
 *
 * 	The thread is created during construction time but only enters the
 * 	specified functions after start() has been called. 
 **/
class Thread
{
	// forbid copy-assignment
	Thread(const Thread&) = delete;
	Thread& operator=(const Thread&) = delete;

public: // types

	/**
	 * \brief
	 * 	Allowed states for the thread
	 * \details
	 * 	Possible lifecycles of a Thread are as follows:
	 *
	 * 	DEAD (thread construction error)
	 *
	 * 	DEAD -> READY -> DEAD (thread was constructed but never
	 * 	started)
	 *
	 * 	DEAD -> READY -> RUN -> EXIT -> DEAD (thread was constructed,
	 * 	started, has exited and was joined)
	 **/
	enum State
	{
		// thread is created but has not yet been started by the client
		READY,
		// run and perform operation
		RUN,
		// stop operation and exit
		EXIT,
		// thread never was successfully created or has exited and was joined.
		DEAD
	};

public: // functions

	/**
	 * \brief
	 * 	Creates a thread
	 * \details
	 * 	No special parameters needed yet.
	 *
	 *	All ressources will be allocated and the thread ready to
	 *	perform client tasks.
	 *
	 *	On error xwmfs::Exception will be thrown.
	 **/
	Thread(IThreadEntry &entry, const char *name = nullptr);

	~Thread();

	State getState() const { return this->m_state; }

	//! make the thread enter the client function
	void start() { this->setState( RUN ); }

	/**
	 * \brief
	 * 	Mark the thread state as EXIT
	 * \details
	 * 	If the thread is currently inside client code then the client
	 * 	code is responsible for reacting to this state change.
	 * 	
	 **/
	void requestExit() { this->setState( EXIT ); }

	//! waits until thread leaves the client function and terminates, sets
	//! state to EXIT
	void join();

protected: // functions

	//! \brief
	//! Changes the thread state to \c s and signals the condition to wake
	//! up a possibly waiting thread
	void setState(const State s)
	{
		{
			MutexGuard g(m_state_lock);
			m_state = s;
		}

		m_state_condition.signal();
	}

	//! POSIX / C entry function for thread. From here the virtual
	//! threadEntry() will be called
	static void* posixEntry(void *par);
	
private: // data
	//! POSIX thread handle
	pthread_t m_pthread;

	//! The current state the thread is in
	State m_state;

	//! Safe multi-threaded access to m_state
	Mutex m_state_lock;

	//! Safe waiting for changes to m_state
	Condition m_state_condition;

	//! The interface for the thread to run in
	IThreadEntry &m_entry;

	//! name of the thread
	std::string m_name;
};

} // end ns

#endif // inc. guard

