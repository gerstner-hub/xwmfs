#include <assert.h>

#include "common/Thread.hxx"
#include "main/StdLogger.hxx"

namespace xwmfs
{
	
Thread::Thread(IThreadEntry &entry, const char *name) :
	m_state(READY),
	m_state_condition(m_state_lock),
	m_entry(entry),
	// we could also use a counter to make unique anonymous
	// threads
	m_name( name ? name : "anonymous" )
{
	const int error = ::pthread_create(
		&m_pthread,
		nullptr /* keep default attributes */,
		&posixEntry,
		(void*)this /* pass pointer to object instance */
	);

	if( error )
	{
		m_state = DEAD;
		xwmfs_throw(
			xwmfs::SystemException("Unable to create thread")
		);
	}
}

Thread::~Thread()
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
		assert("destroy_success" == nullptr);
	}
}
	
void* Thread::posixEntry(void *par)
{
	auto &thread = *(reinterpret_cast<Thread*>(par));

	{
		MutexGuard g(thread.m_state_lock);

		// wait for some state change away from READY before
		// we actually run
		while( thread.getState() == READY )
		{
			thread.m_state_condition.wait();
		}
	}
		
	if( thread.getState() == RUN )
	{
		try
		{
			// enter client function
			(thread.m_entry).threadEntry(thread);
		}
		catch( const xwmfs::Exception &e )
		{
			xwmfs::StdLogger::getInstance().error()
				<< "Caught exception in " << __FUNCTION__
				<< ", thread name = \"" << thread.m_name
				<< "\".\nException: "
				<< e.what() << "\n";
		}
		catch( ... )
		{
			xwmfs::StdLogger::getInstance().error()
				<< "Caught unknown exception in " << __FUNCTION__
				<< ", thread name = \"" << thread.m_name << "\".\n";
		}
	}

	return nullptr;
}
	
void Thread::join()
{
	this->setState( EXIT );

	void *res = nullptr;
	const int join_res = ::pthread_join( m_pthread, &res );

	this->setState( DEAD );

	if( join_res != 0 )
	{
		xwmfs_throw(
			SystemException("Failed to join thread")
		);
	}
}

} // end ns

