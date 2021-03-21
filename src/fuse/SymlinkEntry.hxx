#ifndef XWMFS_SYMLINK_ENTRY_HXX
#define XWMFS_SYMLINK_ENTRY_HXX

// xwmfs
#include "fuse/DirEntry.hxx"
#include "fuse/SymlinkEntry.hxx"

namespace xwmfs
{

class SymlinkEntry :
	public Entry
{
public:
	SymlinkEntry(
		const std::string &n,
		const std::string &target = std::string(),
		const time_t &t = 0) :
			Entry(n, SYMLINK, t),
			m_target(target)
	{ }

	void setTarget(const std::string &target)
	{
		m_target = target;
	}

	void getStat(struct stat*) override;

	virtual int readlink(char *buf, size_t size) override;

protected: // data

	//! target file system location
	std::string m_target;
};

} // end ns

#endif // inc. guard
