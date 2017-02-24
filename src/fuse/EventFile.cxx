// xwmfs
#include "fuse/EventFile.hxx"

namespace xwmfs
{

EventFile::EventFile(
	const std::string &name, const time_t time,
	const size_t max_backlog
) :
	Entry(name, REG_FILE, time),
	m_max_backlog(max_backlog)
{

}

int EventFile::read(char *buf, size_t size, off_t offset)
{
	(void)buf;
	(void)size;
	(void)offset;
	return -ENOSPC;
}

int EventFile::write(const char *buf, size_t size, off_t offset)
{
	(void)buf;
	(void)size;
	(void)offset;
	return -EINVAL;
}

} // end ns

