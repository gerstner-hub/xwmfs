// POSIX
#include <sys/stat.h>

// C++
#include <algorithm>
#include <cstring>

// xwmfs
#include "fuse/SymlinkEntry.hxx"

namespace xwmfs {

void SymlinkEntry::getStat(struct stat *s) const {
	Entry::getStat(s);
	MutexGuard g{m_parent->getLock()};

	s->st_size = m_target.size();
}

int SymlinkEntry::readlink(char *buf, size_t size) {
	if (size == 0)
		return 0;

	auto copylen = std::min(size-1, m_target.size());
	std::memcpy(buf, &m_target[0], copylen);
	buf[copylen] = '\0';
	return 0;
}

} // end ns
