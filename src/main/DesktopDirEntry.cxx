// xwmfs
#include "fuse/FileEntry.hxx"
#include "main/DesktopDirEntry.hxx"

namespace xwmfs
{

DesktopDirEntry::DesktopDirEntry(size_t nr, const std::string &name) :
	DirEntry(std::to_string(nr)),
	m_nr(nr), m_name(name)
{
	auto name_node = new FileEntry("name");

	*name_node << name << "\n";

	this->addEntry(name_node);

	this->addEntry( new DirEntry("windows") );
}

} // end ns
