#ifndef XWMFS_THREAD_HXX
#define XWMFS_THREAD_HXX

#include <pthread.h>

#include "common/SystemException.hxx"
#include "common/Mutex.hxx"
#include "common/Condition.hxx"
#include "common/IThreadEntry.hxx"

#include "main/StdLogger.hxx"

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
	Thread(IThreadEntry &entry, const char *name = NULL) :
		m_state(READY),
		m_state_condition(m_state_lock),
		m_entry(entry),
		// we could also use a counter to make unique anonymous
		// threads
		m_name( name ? name : "anonymous" )
	{
		const int res = ::pthread_create(
			&m_pthread,
			NULL /* keep default attributes */,
			&posixEntry,
			(void*)this /* pass pointer to object instance */
		);

		if( res )
		{
			m_state = DEAD;
			xwmfs_throw(
				xwmfs::SystemException("Unable to create thread")
			);
		}
	}

	~Thread()
	{
		try
		{
			assert( m_state != RUN && m_state != EXIT );

			if( m_state == READY )
			{
				// join thread first
				this->join();
			}
		}
		catch( ... )
		{
			assert("destroy_success" == NULL);
		}
	}

	State getState() const
	{ return this->m_state; }

	//! make the thread enter the client function
	void start()
	{
		this->setState( RUN );
	}

	/**
	 * \brief
	 * 	Mark the thread state as EXIT
	 * \details
	 * 	If the thread is currently inside client code then the client
	 * 	code is responsible for reacting to this state change.
	 * 	
	 **/
	void requestExit()
	{
		this->setState( EXIT );
	}

	//! \brief
	//! waits until thread leaves the client function and terminates, sets
	//! state to EXIT
	void join()
	{
		this->setState( EXIT );

		void *res;

		const int join_res = ::pthread_join( m_pthread, &res );

		assert( ! join_res );

		this->setState( DEAD );
	}

protected: // functions

	//! \brief
	//! Changes the thread state to \c s and signals the condition to wake
	//! up a possibly waiting thread
	void setState(const State s)
	{
		m_state_lock.lock();

		m_state = s;

		m_state_lock.unlock();

		m_state_condition.signal();
	}

	//! \brief
	//! POSIX / C entry function for thread. From here the virtual
	//! threadEntry() will be called
	static void* posixEntry(void *par)
	{
		Thread *inst = reinterpret_cast<Thread*>(par);

		inst->m_state_lock.lock();

		while( inst->getState() == READY )
		{
			inst->m_state_condition.wait();
		}
			
		inst->m_state_lock.unlock();
			
		if( inst->getState() == RUN )
		{
			try
			{
				// enter client function
				(inst->m_entry).threadEntry(*inst);
			}
			catch( const xwmfs::Exception &e )
			{
				xwmfs::StdLogger::getInstance().error()
					<< "Caught exception in " << __FUNCTION__
					<< ", thread name = \"" << inst->m_name
					<< "\".\nException: "
					<< e.what() << "\n";
			}
			catch( ... )
			{
				xwmfs::StdLogger::getInstance().error()
					<< "Caught unknown exception in " << __FUNCTION__
					<< ", thread name = \"" << inst->m_name << "\".\n";
			}
		}

		return NULL;
	}
	
	// protected copy ctor. and assignment op. to avoid copies
	Thread(const Thread&);
	Thread& operator=(const Thread&);

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

