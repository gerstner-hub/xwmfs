// xwmfs
#include "fuse/xwmfs_fuse.hxx"
#include "main/Xwmfs.hxx"

// C++
#include <string>
#include <iostream>

// POSIX
#include <unistd.h>
#include <sys/stat.h>

namespace xwmfs
{

const uid_t Entry::m_uid = ::getuid();
const gid_t Entry::m_gid = ::getgid();

Entry* RootEntry::findEntry(const char* path)
{
	// should start with the root
	assert(path[0] == '/');

	// the current directory entry we're looking at - start with ourselves, of course
	xwmfs::DirEntry *cur_dir = this;
	// the current path entity we're looking at
	std::string cur_entity;
	// start of the current path entity string
	const char *entity_start = path + 1;
	// end of the current path entity string (at the path separator or end
	// of string)
	const char *entity_end = strchrnul(entity_start, '/');
	// whether this is the last path entity in \c path
	bool last_entity = (*entity_end == '\0');

	// matching return entry, if any
	// if "/" is queried then we immediatly know the result here
	xwmfs::Entry *ret = (entity_start == entity_end ? this : nullptr);

	// if start equals the end then the last entity has been reached
	while( entity_start != entity_end )
	{
		// assign the current entity to new string. this costs a
		// single or few heap allocations per call and a copy
		// operation per entity but otherwise we couldn't get a decent
		// null terminator at the end of the entity
		//
		// XXX could be done via strncmp for better performance
		cur_entity.assign( entity_start, entity_end - entity_start );

		if( ! last_entity )
		{
			cur_dir = xwmfs::Entry::tryCastDirEntry(cur_dir->getEntry(cur_entity));
			// either the entity is no directory or not existing
			if( !cur_dir )
			{
				return nullptr;
			}
		}
		else
		{
			// the last entity may also be a file instead of a
			// directory
			ret = cur_dir->getEntry(cur_entity);
			// last entity part is not existing
			if( ! ret )
			{
				return nullptr;
			}
		}

		// the next entity starts one character after current
		// entity_end, or if already reached the end then set equal to
		// entity_end
		entity_start = ( last_entity ? entity_end : entity_end + 1 );

		if( ! last_entity )
		{
			entity_end = strchrnul(entity_start, '/');

			// the second check is for cases of "/some/path/" with
			// a trailing slash
			if( *entity_end == '\0' || *(entity_end + 1) == '\0')
			{
				last_entity = true;
				// this is for cases with an ending '/'
				if( entity_start == entity_end )
					ret = cur_dir;
			}
		}
	}

	// we should have found a return entry if we reach this part of the
	// code
	assert( ret );

	return ret;
}

int Entry::parseInteger(const char *data, const size_t bytes, int &result) const
{
	size_t endpos;
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

void FileEntry::getStat(struct stat *s)
{
	Entry::getStat(s);
		
	// determine size of stream and return it in stat structure
	this->seekg( 0, xwmfs::FileEntry::end );
	s->st_size = this->tellg();
}

} // end ns

