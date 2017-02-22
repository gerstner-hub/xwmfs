// POSIX
#include <sys/stat.h>

// xwmfs
#include "fuse/FileEntry.hxx"

namespace xwmfs
{

void FileEntry::getStat(struct stat *s)
{
	Entry::getStat(s);
		
	// determine size of stream and return it in stat structure
	this->seekg( 0, xwmfs::FileEntry::end );
	s->st_size = this->tellg();
}

} // end ns

