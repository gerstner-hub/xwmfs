#pragma once

// C++
#include <deque>

// cosmos
#include <cosmos/thread/Condition.hxx>

// xwmfs
#include "fuse/Entry.hxx"

namespace xwmfs {

// forward declaration
struct EventOpenContext;

/// A special file that allows readers to block until new events arrive.
/**
 * While all other file system entries contain some small and defined amount
 * of data, an EventFile offers a potentially endless stream of data as new
 * events are coming in.
 *
 * Events can be arbitrary strings to be delivered to readers. Multiple
 * readers may block on an EventFile until new data arrives.
 *
 * If a reader is too slow to catch up with new events then it'll loose
 * some events in between without noticing.
 **/
class EventFile :
		public Entry {
public:

	/// Creates a new event file using the given name and initial timestamp.
	/**
	 * \param[in] max_backlog
	 * 	Determines the maximum number of events that an active reader
	 * 	may have in backlog before it's loosing the oldest events.
	 * 	This is necessary to avoid infinite growth of the event queue
	 * 	in case an active reader doesn't catch up with the data.
	 **/
	EventFile(DirEntry &parent, const std::string &name, const cosmos::RealTime &time = cosmos::RealTime{},
			const size_t max_backlog = 64);

	/// Creates an extended OpenContext with additional EventFile context data.
	OpenContext* createOpenContext() override;

	/// Adds a new event for potential readers to receive.
	void addEvent(const std::string &text);

	bool enableDirectIO() const override { return true; }

	/// We always allow operations on event files, even if closed.
	/**
	 * This is to allow reads to see the final destroy event.
	 **/
	int isOperationAllowed() const override { return 0; }

public: // types

	struct Event {
		enum class ID : size_t {
			INVALID = SIZE_MAX
		};

		std::string text;
		ID id = ID{0};

		Event(const std::string &s, ID _id) :
			text{s}, id{_id} {
		}
	};

	using EventQueue = std::deque<Event>;

protected: // functions

	bool markDeleted() override;

	Bytes read(OpenContext *ctx, char *buf, size_t size, off_t offset) override;
	Bytes write(OpenContext *ctx, const char *buf, size_t size, off_t offset) override;

	/// Returns a pointer to the next event in the queue coming after the event with `prev_id`.
	/**
	 * This must be called with the parent lock held. Returns nullptr if
	 * there is no next event yet. Events between `prev_id` and the
	 * returned event may have been skipped if the reader took too much
	 * time.
	 **/
	const Event* nextEvent(const Event::ID prev_id);

	int readEvent(EventOpenContext &ctx, char *buf, size_t size);

	Event::ID nextID();

protected: // data

	const size_t m_max_backlog;
	cosmos::Condition m_cond;
	EventQueue m_event_queue;
	Event::ID m_next_id = Event::ID{0};
};

} // end ns
