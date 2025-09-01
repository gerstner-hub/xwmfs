#pragma once

// xwmfs
#include "fuse/DirEntry.hxx"
#include "fuse/SymlinkEntry.hxx"

namespace xwmfs {

class SymlinkEntry :
		public Entry {
public:
	SymlinkEntry(const std::string &n,
				const std::string &target = std::string(),
				const cosmos::RealTime t = cosmos::RealTime{}) :
			Entry{n, SYMLINK, t},
			m_target{target} {
	}

	void setTarget(const std::string &target) {
		m_target = target;
	}

	void getStat(struct stat*) const override;

	virtual int readlink(char *buf, size_t size) override;

protected: // data

	/// target file system location
	std::string m_target;
};

} // end ns
