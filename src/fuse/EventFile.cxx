// xwmfs
#include "fuse/EventFile.hxx"
#include "fuse/OpenContext.hxx"
#include "fuse/DirEntry.hxx"
#include "fuse/AbortHandler.hxx"
#include "fuse/xwmfs_fuse.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs
{

const size_t INVAL_ID = SIZE_MAX;

struct EventOpenContext :
	public OpenContext
{
	EventOpenContext(Entry *entry, const size_t first_id) :
		OpenContext(entry),
		cur_id(first_id)
	{}

	EventOpenContext(const EventOpenContext&) = delete;

	/*
	 * instead of storing a shared ptr and an offset to the event, reduce
	 * our functionality to providing complete events only. Thus we can
	 * switch to a std::deque of fixed maximum size.
	 *
	 * only question is about the kind of errno signaling. There's no
	 * error to say "your buffer is too small". Because everything is
	 * character/byte based. But there are cases like readlink(2) which
	 * silently truncates in such cases. That's what we do.
	 */

	//! the next event ID to present the reader
	size_t cur_id = INVAL_ID;
};

EventFile::EventFile(
	DirEntry &parent,
	const std::string &name, const time_t time,
	const size_t max_backlog
) :
	Entry(name, REG_FILE, false, time),
	m_max_backlog(max_backlog),
	m_cond(parent.getLock())
{
	this->createAbortHandler(m_cond);
}

bool EventFile::markDeleted()
{
	bool ret;

	{
		MutexGuard g(m_parent->getLock());
		ret = Entry::markDeleted();
	}

	// make sure any blocked readers notice we're gone
	m_cond.broadcast();

	return ret;
}

void EventFile::addEvent(const std::string &text)
{
	{
		MutexGuard g(m_parent->getLock());

		// reflect the most recent event time as modification time
		this->setModifyTime(Xwmfs::getInstance().getCurrentTime());

		if( ! m_refcount )
		{
			// no readers, so nothing to do
			return;
		}

		if( m_event_queue.size() == m_max_backlog )
		{
			m_event_queue.pop_front();
		}

		m_event_queue.push_back( Event(text, m_next_id++) );

		if( m_next_id == INVAL_ID )
		{
			m_next_id = 0;
		}
	}

	// wake up all readers so they can read the new event, if required
	m_cond.broadcast();
}

const EventFile::Event* EventFile::nextEvent(const size_t prev_id)
{
	if( m_event_queue.empty() )
	{
		return nullptr;
	}

	const auto num_events = m_event_queue.size();
	const auto oldest_id = m_event_queue[0].id;
	const auto newest_id = m_event_queue[num_events-1].id;

	if( prev_id == INVAL_ID )
	{
		// no previous event available, return the oldest one then
		return &m_event_queue[0];
	}
	else if( newest_id == prev_id )
	{
		// no new event available
		return nullptr;
	}
	else if( oldest_id > newest_id )
	{
		// id wraparound situation, fallback to linear search to find
		// the right current event
		for( size_t i = 0; i < num_events; i++ )
		{
			const auto &event = m_event_queue[i];

			if( event.id == prev_id )
			{
				return &(m_event_queue[i+1]);
			}
		}
	}
	else if( oldest_id > prev_id )
	{
		// some events have been lost for this reader
		return &(m_event_queue[0]);
	}

	const auto index = prev_id - oldest_id + 1;

	return &(m_event_queue[index]);
}

int EventFile::readEvent(EventOpenContext &ctx, char *buf, size_t size)
{
	const Event *event = nullptr;

	MutexGuard g(m_parent->getLock());

	while( (event = nextEvent(ctx.cur_id)) == nullptr )
	{
		if( this->isDeleted() )
		{
			// file was closed in the meantime, signal EOF
			return 0;
		}
		else if( ctx.isNonBlocking() )
		{
			// don't block if there's no event immediately
			// available
			return -EAGAIN;
		}
		else if( m_abort_handler->wasAborted() )
		{
			return -EINTR;
		}

		if( ! m_abort_handler->prepareBlockingCall(this) )
		{
			return -EINTR;
		}
		m_cond.wait();
		m_abort_handler->finishedBlockingCall();
	}

	const auto copy_size = std::min( (event->text).size(), size - 1);
	std::memcpy(buf, (event->text).c_str(), copy_size);
	// ship a newline after each event
	buf[copy_size] = '\n';

	ctx.cur_id = event->id;

	return copy_size + 1;
}

int EventFile::read(OpenContext *ctx, char *buf, size_t size, off_t offset)
{
	// we ignore the read offset, because we return records depending on
	// the state of the open context here
	(void)offset;
	auto &evt_ctx = *(reinterpret_cast<EventOpenContext*>(ctx));

	auto &fs_lock = xwmfs::Xwmfs::getInstance().getFS();

	/*
	 * This is a pretty stupid situation here. we need to release
	 * the global filesystem read lock here to avoid deadlocks.
	 * For example if the caller wants to close its file
	 * descriptor while being blocked here, or practically any
	 * other operation.
	 *
	 * Now that we have a mutex per directory we might just use
	 * that one instead or the global lock to protect read
	 * operations from write operations or events.
	 *
	 * the read lock is actually not required at all for
	 * EventFile for reading, but in general is required for other
	 * entry types and is taken in xwmfs_read early on.
	 */

	FileSysRevReadGuard guard(fs_lock);
	const int ret = readEvent(evt_ctx, buf, size);
	return ret;
}

int EventFile::write(OpenContext *ctx, const char *buf, size_t size, off_t offset)
{
	/*
	 * writing to an event file is not supported at all
	 */
	(void)ctx;
	(void)buf;
	(void)size;
	(void)offset;
	return -EINVAL;
}

OpenContext* EventFile::createOpenContext()
{
	auto ret = new EventOpenContext(
		this,
		// make sure no outdated events are presented to new readers
		m_event_queue.empty() ? INVAL_ID : m_event_queue.back().id
	);

	this->ref();

	return ret;
}

} // end ns
