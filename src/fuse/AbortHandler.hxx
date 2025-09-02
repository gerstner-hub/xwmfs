#pragma once

// C++
#include <set>

// cosmos
#include <cosmos/thread/Condition.hxx>
#include <cosmos/thread/pthread.hxx>

namespace xwmfs {

class Entry;

/// Blocking call abort helper for Entry.
/**
 * If an Entry implementation needs to support blocking calls then this mixin
 * helps dealing with its complexities.
 *
 * Functions in Entry and the logic in the Xwmfs main class work together with
 * this mixin.
 **/
class AbortHandler {
public: // functions

	AbortHandler(cosmos::Condition &cond);

	/// Returns whether the calling thread should abort its operation.
	/**
	 * The information will be deleted after calling this function, so a
	 * subsequent call will return `false`.
	 *
	 * This function needs to be called with the mutex associated
	 * with m_cond held!
	 **/
	bool wasAborted();

	/// Records an abort request for the given thread and wakes up associated threads.
	void abort(const cosmos::pthread::ID thread);

	/// Call this before a blocking call is about to be executed.
	/**
	 * \return
	 * `true` if registration was successful, `false` if the current
	 * program state doesn't allow execution of blocking calls.
	 **/
	bool prepareBlockingCall(Entry *file);

	/// Call this after a blocking call has finished.
	void finishedBlockingCall();

protected: // types

	using AbortSet = std::set<cosmos::pthread::ID>;

protected: // data

	cosmos::Condition &m_cond;
	/// Records threads for which blocking calls shall be aborted.
	AbortSet m_abort_set;
};

} // end ns
