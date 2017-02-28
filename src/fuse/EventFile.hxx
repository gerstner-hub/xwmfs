#ifndef XWMFS_EVENT_FILE_HXX
#define XWMFS_EVENT_FILE_HXX

// xwmfs
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
	 * 	This is necessary to avoid infinite growths of the event queue
	 * 	in case an active reader doesn't catch up the data
	 **/
	EventFile(
		const std::string &name,
		const time_t time,
		const size_t max_backlog = 64
	);

	/**
	 * \brief
	 * 	Creates an extended OpenContext with additional EventFile
	 * 	context data
	 **/
	OpenContext* createOpenContext() override;

protected: // functions

	int read(OpenContext *ctx, char *buf, size_t size, off_t offset) override;
	int write(OpenContext *ctx, const char *buf, size_t size, off_t offset) override;

protected: // types

	struct Event
	{
		std::string text;
		size_t id = 0;
	};

protected: // data

	const size_t m_max_backlog;
	size_t m_next_id = 1;
};

} // end ns

#endif // inc. guard

