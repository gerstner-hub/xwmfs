// xwmfs
#include "main/SelectionAccessFile.hxx"
#include "main/SelectionDirEntry.hxx"

namespace xwmfs
{

SelectionAccessFile::SelectionAccessFile(
	const std::string &n,
	const SelectionDirEntry &parent,
	const XAtom &type
) :
	FileEntry(n, true, 0),
	m_parent(parent),
	m_type(type)
{
}

} // end ns

