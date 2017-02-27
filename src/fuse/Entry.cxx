// C++
#include <string>
#include <iostream>

// POSIX
#include <sys/stat.h>
#include <unistd.h>

// xwmfs
#include "fuse/Entry.hxx"
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

	if( this->isDir() )
	{
		s->st_mode = S_IFDIR | 0755;
		// a directory is always linked at least twice due to '.'
		s->st_nlink = 2;
	}
	else if( this->isRegular() )
	{
		s->st_mode = S_IFREG | (this->isWritable() ? 0664 : 0444);
		s->st_nlink = 1;
	}
	else
	{
		// ???
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

} // end ns

