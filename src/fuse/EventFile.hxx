#ifndef XWMFS_EVENT_FILE_HXX
#define XWMFS_EVENT_FILE_HXX

// C++
#include <vector>
#include <set>

// xwmfs
#include "common/Condition.hxx"
#include "fuse/Entry.hxx"

namespace xwmfs
{

// fwd. decl.
struct EventOpenContext; 

/**
 * \brief
 * 	A special file that allows readers to block until new events arrive
 * \details
 * 	While all other file system entries contain some small defined amount
 * 	of data, an event file offers a potentielly endless stream of data as
 * 	new events are coming in.
 *
 * 	Events can be arbitrary strings to be delivered to readers. Multiple
 * 	readers may block on an EventFile until new data arrives.
 *
 * 	If a reader is too slow to catch up with new events then it'll loose
 * 	some events in between without noticing.
 **/
class EventFile :
	public Entry
{
public:

	/**
	 * \brief
	 * 	Creates a new event file of the given name and initial
	 * 	timestamps
	 * \param[in] max_backlog
	 * 	Determines the maximum number of events that an active reader
	 * 	may have in backlog before he's loosing the oldest events.
	 * 	This is necessary to avoid infinite growth of the event queue
	 * 	in case an active reader doesn't catch up with the data
	 **/
	EventFile(
		DirEntry &parent,
		const std::string &name,
		const time_t time = 0,
		const size_t max_backlog = 64
	);

	/**
	 * \brief
	 * 	Creates an extended OpenContext with additional EventFile
	 * 	context data
	 **/
	OpenContext* createOpenContext() override;

	/**
	 * \brief
	 * 	Adds a new event for potential readers to receive
	 **/
	void addEvent(const std::string &text);

	bool enableDirectIO() const override { return true; }

	/**
	 * \brief
	 * 	Abort an ongoing blocking read call, if any
	 **/
	void abortBlockingCall(pthread_t thread);

public: // types

	struct Event
	{
		std::string text;
		size_t id = 0;

		Event(const std::string &s, size_t i) : text(s), id(i) {}
	};

	typedef std::vector<Event> EventQueue;
	typedef std::set<pthread_t> AbortSet;

protected: // functions

	bool markDeleted() override;

	int read(OpenContext *ctx, char *buf, size_t size, off_t offset) override;
	int write(OpenContext *ctx, const char *buf, size_t size, off_t offset) override;

	/**
	 * \brief
	 * 	Returns a pointer to the next event in the queue coming after
	 * 	the event with id \c prev_id
	 * \details
	 * 	Must be called with the parent lock held. Returns nullptr if
	 * 	there is no next event yet. Events between \c prev_id and the
	 * 	return event may have been skipped if the reader took too much
	 * 	time.
	 **/
	const Event* nextEvent(size_t prev_id);

protected: // data

	const size_t m_max_backlog;
	Condition m_cond;
	EventQueue m_event_queue;
	size_t m_next_id = 1;
	//! holds threads for which blocking calls shall be aborted
	AbortSet m_abort_set;
};

} // end ns

#endif // inc. guard

