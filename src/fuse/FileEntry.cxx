// POSIX
#include <sys/stat.h>

// xwmfs
#include "fuse/DirEntry.hxx"
#include "fuse/FileEntry.hxx"

namespace xwmfs {

void FileEntry::getStat(struct stat *s) const {
	Entry::getStat(s);
	cosmos::MutexGuard g{m_parent->getLock()};

	/*
	 * we are modifying the stream position here, but that isn't
	 * problematic, since we'll always reposition it during read or write.
	 */
	auto &stream = static_cast<std::stringstream&>(const_cast<FileEntry&>(*this));

	// determine size of stream and return it in stat structure
	stream.seekg(0, xwmfs::FileEntry::end);
	s->st_size = stream.tellg();
}

FileEntry::Bytes FileEntry::write(OpenContext *ctx, const char *data, size_t size, off_t offset) {
	(void)ctx;
	(void)data;
	(void)size;
	(void)offset;
	throw cosmos::Errno::OP_NOT_SUPPORTED;
}

FileEntry::Bytes FileEntry::read(OpenContext *ctx, char *buf, size_t size, off_t offset) {
	(void)ctx;
	cosmos::MutexGuard g{m_parent->getLock()};

	// position to the required offset in the file (to beginning of file, if no offset)
	seekg(offset, xwmfs::FileEntry::beg);

	// read data into FUSE buffer
	static_cast<std::stringstream&>(*this).read(buf, size);

	const int ret = gcount();

	// remove any possible error or EOF states from stream
	clear();

	// return number of bytes actually retrieved
	return Bytes{ret};
}

} // end ns
