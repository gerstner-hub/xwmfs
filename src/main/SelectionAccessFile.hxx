#ifndef XWMFS_SELECTIONACCESSFILE_HXX
#define XWMFS_SELECTIONACCESSFILE_HXX

// xwmfs
#include "fuse/FileEntry.hxx"
#include "x11/XAtom.hxx"

namespace xwmfs
{

class SelectionDirEntry;

/**
 * \brief
 * 	This file provides access to an arbitrary X selection buffer
 * \details
 * 	This file type makes it possible to
 *
 * 	- upon write, take ownership of a selection buffer and provide the
 * 	written contents to other X clients
 * 	- upon read, request the selection buffer content from the current
 * 	selection owner and return the data to the user
 *
 * 	for any available type of X selection buffer.
 **/
class SelectionAccessFile :
	public FileEntry
{
public: // functions

	/**
	 * \brief
	 * 	Creates a new selection access file of the given name and type
	 * \param[in] parent
	 * 	The parent directory which needs to be of type
	 * 	SelectionDirEntry
	 * \param[in] type
	 * 	The atom identifier of the selection type this file entry
	 * 	should handle
	 **/
	SelectionAccessFile(
		const std::string &n,
		const SelectionDirEntry &parent,
		const XAtom &type
	);

protected: // data

	//! parent dir with helper routines
	const SelectionDirEntry &m_parent;
	//! the X selection type we represent
	const XAtom m_type;
};

} // end ns

#endif // inc. guard

