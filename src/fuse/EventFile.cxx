// xwmfs
#include "fuse/EventFile.hxx"
#include "fuse/OpenContext.hxx"
#include "fuse/DirEntry.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs
{

const size_t INVAL_ID = 0;

struct EventOpenContext :
	public OpenContext
{
	EventOpenContext(Entry *entry) :
		OpenContext(entry)
	{}

	/*
	 * instead of storing a shared ptr and an offset to the event reduce
	 * our functionality to providing complete events only. Thus we can
	 * switch to a std::vector of fixed reserved size.
	 *
	 * only question is about the kind of errno signaling. There's no
	 * error to say "you buffer is too small". Because everything is
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
	m_event_queue.reserve(m_max_backlog);
}

bool EventFile::markDeleted()
{
	bool ret;

	{
		MutexGuard g(m_parent->getLock());
		ret = Entry::unref();
	}

	// make sure any blocked readers notice we're gone
	m_cond.broadcast();

	return ret;
}

void EventFile::addEvent(const std::string &text)
{
	{
		MutexGuard g(m_parent->getLock());
		if( ! m_refcount )
		{
			// no readers, so nothing to do
			return;
		}

		m_event_queue.push_back( Event(text, m_next_id++) );

		if( m_next_id == INVAL_ID )
		{
			m_next_id++;
		}

		// reflect the most recent event time as modification time
		this->setModifyTime(Xwmfs::getInstance().getCurrentTime());
	}

	// wake up all readers so they can read the new event, if required
	m_cond.broadcast();
}

const EventFile::Event* EventFile::nextEvent(const size_t prev_id)
{
	if( m_event_queue.empty() )
		return nullptr;

	const auto num_events = m_event_queue.size();
	const auto oldest_id = m_event_queue[0].id;
	const auto newest_id = m_event_queue[num_events-1].id;

	if( newest_id == prev_id )
		// no new event available
		return nullptr;

	if( oldest_id > newest_id )
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

int EventFile::read(OpenContext *ctx, char *buf, size_t size, off_t offset)
{
	// we ignore the read offset, because we return records depending on
	// the state of the open context here
	(void)offset;
	auto evt_ctx = *(reinterpret_cast<EventOpenContext*>(ctx));
	const Event *event = nullptr;

	auto &xwmfs = xwmfs::Xwmfs::getInstance();
	auto &fs_lock = xwmfs.getFS();
	const auto self = pthread_self();

	MutexGuard g(m_parent->getLock());

	while( (event = nextEvent(evt_ctx.cur_id)) == nullptr )
	{
		if( this->isDeleted() )
		{
			// file was closed in the meantime
			return -EBADF;
		}
		else if( evt_ctx.isNonBlocking() )
		{
			// don't block if there's no event immediately
			// available
			return -EAGAIN;
		}
		else if( m_abort_set.find( self ) != m_abort_set.end() )
		{
			m_abort_set.erase(self);
			return -EINTR;
		}

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

		fs_lock.unlock();
		xwmfs.registerBlockingCall(this);
		m_cond.wait();
		xwmfs.unregisterBlockingCall();
		fs_lock.readlock();
	}

	const auto copy_size = std::min( (event->text).size(), size - 1);
	std::memcpy(buf, (event->text).c_str(), copy_size);
	// ship a newline after each event
	buf[copy_size] = '\n';

	evt_ctx.cur_id = event->id;

	return copy_size + 1;
}

void EventFile::abortBlockingCall(pthread_t thread)
{
	{
		MutexGuard g(m_parent->getLock());
		m_abort_set.insert( thread );
	}

	m_cond.broadcast();
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
	auto ret = new EventOpenContext(this);

	this->ref();

	return ret;
}

} // end ns
