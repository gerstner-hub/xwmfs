// POSIX
#include <sys/stat.h>

// xwmfs
#include "fuse/FileEntry.hxx"
#include "fuse/DirEntry.hxx"

namespace xwmfs
{

void FileEntry::getStat(struct stat *s)
{
	Entry::getStat(s);
	MutexGuard g(m_parent->getLock());

	// determine size of stream and return it in stat structure
	this->seekg( 0, xwmfs::FileEntry::end );
	s->st_size = this->tellg();
}

int FileEntry::write(const char *data, size_t size, off_t offset)
{
	(void)data;
	(void)size;
	(void)offset;
	return -EINVAL;
}

int FileEntry::read(char *buf, size_t size, off_t offset)
{
	MutexGuard g(m_parent->getLock());

	// position to the required offset in the file (to beginning of file, if no offset)
	seekg( offset, xwmfs::FileEntry::beg );

	// read data into fuse buffer
	static_cast<std::stringstream&>(*this).read( buf, size );

	const int ret = gcount();

	// remove any possible error or EOF states from stream
	clear();

	// return number of bytes actually retrieved
	return ret;
}

} // end ns

