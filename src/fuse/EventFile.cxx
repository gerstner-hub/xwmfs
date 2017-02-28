// xwmfs
#include "fuse/EventFile.hxx"
#include "fuse/OpenContext.hxx"

namespace xwmfs
{

struct EventOpenContext :
	public OpenContext
{
	EventOpenContext(Entry *entry) :
		OpenContext(entry)
	{}

	size_t next_id = 0;
};

EventFile::EventFile(
	const std::string &name, const time_t time,
	const size_t max_backlog
) :
	Entry(name, REG_FILE, time),
	m_max_backlog(max_backlog)
{

}

int EventFile::read(OpenContext *ctx, char *buf, size_t size, off_t offset)
{
	auto evt_ctx = *(reinterpret_cast<EventOpenContext*>(ctx));

	if(offset)
	{
		return -EOPNOTSUPP;
	}

	evt_ctx.next_id++;

	return snprintf(buf, size, "%zd", evt_ctx.next_id);
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

