#ifndef XWMFS_DESKTOP_DIR_ENTRY_HXX
#define XWMFS_DESKTOP_DIR_ENTRY_HXX

// xwmfs
#include "fuse/DirEntry.hxx"

namespace xwmfs
{

class DesktopDirEntry :
	public DirEntry
{
public: // functions

	DesktopDirEntry(size_t nr, const std::string &name);

	size_t getDesktopNr() const { return m_nr; }

	DirEntry* getWindowsDir() { return this->getDirEntry("windows"); }

protected: // functions

	size_t m_nr;
	std::string m_name;
};

} // end ns

#endif // inc. guards
