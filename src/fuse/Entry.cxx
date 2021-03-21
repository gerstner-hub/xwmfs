// C++
#include <string>
#include <iostream>

// POSIX
#include <sys/stat.h>
#include <unistd.h>

// xwmfs
#include "fuse/AbortHandler.hxx"
#include "fuse/Entry.hxx"
#include "fuse/OpenContext.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs
{

const uid_t Entry::m_uid = ::getuid();
const gid_t Entry::m_gid = ::getgid();


Entry::~Entry()
{
	if( !m_parent || m_parent == this )
		// no distict parent
		return;

	if( m_parent->unref() )
	{
		delete m_parent,
		m_parent = nullptr;
	}

	if( m_abort_handler )
	{
		delete m_abort_handler;
		m_abort_handler = nullptr;
	}
}

void Entry::createAbortHandler(Condition &cond)
{
	m_abort_handler = new AbortHandler(cond);
}

void Entry::abortBlockingCall(pthread_t thread)
{
	if( !m_abort_handler )
	{
		return;
	}

	return m_abort_handler->abort(thread);
}

int Entry::parseInteger(const char *data, const size_t bytes, int &result) const
{
	size_t endpos = 0;
	std::string string(data, bytes);

	try
	{
		result = std::stoi( string, &endpos );
	}
	catch( const std::exception &ex )
	{
		std::cerr << ex.what() << std::endl;
		result = -1;
		return -EINVAL;
	}

	return endpos;
}

void Entry::getStat(struct stat *s)
{
	s->st_uid = m_uid;
	s->st_gid = m_gid;
	s->st_atime = s->st_mtime = getModifyTime();
	s->st_ctime = getStatusTime();

	switch(m_type)
	{
	default:
		// ???
		s->st_mode = 0;
		break;
	case DIRECTORY:
		s->st_mode = S_IFDIR | 0755;
		// a directory is always linked at least twice due to '.'
		s->st_nlink = 2;
		break;
	case REG_FILE:
		s->st_mode = S_IFREG | (this->isWritable() ? 0664 : 0444);
		s->st_nlink = 1;
		break;
	case SYMLINK:
		s->st_mode = S_IFLNK | 0777;
		break;
	}

	// apply the current process's umask to the file permissions
	s->st_mode &= ~(Xwmfs::getUmask());
}

int Entry::isOperationAllowed() const
{
	if( isDeleted() )
	{
		// difficult to say what the correct errno for "file
		// disappeared" is. This one seems suitable. It would also be
		// valid to succeed in reading but then the application can't
		// detect the case that the represented object has vanished
		// ...
		return -ENXIO;
	}

	return 0;
}

void Entry::setParent(DirEntry *dir)
{
	m_parent = dir;

	if( this == dir )
		// it's ourselves, no need for reference handling
		return;

	// make sure the parent exists at least as long as us
	m_parent->ref();
}

OpenContext* Entry::createOpenContext()
{
	// TODO: we could improve performance here by using pre-allocated
	// objects for OpenContext
	auto ret = new OpenContext(this);

	// increase the reference count to avoid deletion while the file is
	// opened
	//
	// NOTE: this race conditions only shows if fuse is mounted with the
	// direct_io option, otherwise heavy caching is employed that avoids
	// the SEGFAULT when somebody tries to read from an Entry that's
	// already been deleted
	this->ref();

	return ret;
}

void Entry::destroyOpenContext(OpenContext *ctx)
{
	delete ctx;

	if( this->unref() )
	{
		// deleting this entry should not require a write guard,
		// because we're the last user of the file nobody else should
		// know about it ...
		delete this;
	}
}

} // end ns
