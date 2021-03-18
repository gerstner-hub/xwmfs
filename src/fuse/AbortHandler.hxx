#ifndef XWMFS_ABORT_HANDLER_HXX
#define XWMFS_ABORT_HANDLER_HXX

// C++
#include <set>

// xwmfs
#include "common/Condition.hxx"

namespace xwmfs
{

class Entry;

/**
 * \brief
 * 	Mixin class for Entry
 * \details
 * 	If a certain Entry implementation needs to support blocking calls
 * 	then this mixin helps dealing with its complexities.
 *
 * 	Functions in Entry and the logic in the Xwmfs main class work
 * 	together with this mixin.
 **/
class AbortHandler
{
public: // functions

	AbortHandler(Condition &cond);

	/**
	 * \brief
	 * 	Returns whether the calling thread should abort its operation
	 * \details
	 * 	The information will be deleted after calling this function,
	 * 	so a subsequent call will return false.
	 *
	 * 	This function needs to be called with the mutex associated
	 * 	with m_cond held!
	 **/
	bool wasAborted();

	/**
	 * \brief
	 * 	Records an abort request for the given thread and wakes up
	 * 	associated threads
	 **/
	void abort(pthread_t thread);

	/**
	 * \brief
	 * 	Call this before a blocking call is about to be executed
	 * \return
	 * 	true if registration was successful, false if the current
	 * 	program state doesn't allow execution of blocking calls
	 **/
	bool prepareBlockingCall(Entry *file);

	/**
	 * \brief
	 * 	Call this after a blocking call has finished
	 **/
	void finishedBlockingCall();

protected: // types

	typedef std::set<pthread_t> AbortSet;

protected: // data

	Condition &m_cond;
	//! holds threads for which blocking calls shall be aborted
	AbortSet m_abort_set;
};

} // end ns

#endif // inc. guard
