#ifndef XWMFS_SELECTIONACCESSFILE_HXX
#define XWMFS_SELECTIONACCESSFILE_HXX

// xwmfs
#include "common/Condition.hxx"
#include "fuse/FileEntry.hxx"
#include "x11/XAtom.hxx"
#include "x11/XWindow.hxx"

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
		SelectionDirEntry &parent,
		const XAtom &type
	);

	int read(OpenContext *ctx, char *buf, size_t size, off_t offset) override;

	void reportConversionResult(Atom result_prop);

	//! \see EventFile::enableDirectIO()
	bool enableDirectIO() const override { return true; }

	const XAtom& type() const { return m_sel_type; }

protected: // functions

	/**
	 * \brief
	 * 	Updates the cached owner information in m_owner
	 **/
	void updateOwner();

	/**
	 * \brief
	 * 	Requests the current selection buffer contents and stores it
	 * 	as the file entries data
	 * \return
	 * 	An errno error indication to return to FUSE or zero on success
	 **/
	int updateSelection();

protected: // data

	//! parent dir with helper routines
	SelectionDirEntry &m_parent;
	//! the X selection type we represent
	const XAtom m_sel_type;
	//! property where requested selection buffer conversions go to
	const XAtom m_target_prop;
	//! caches the current owner window of the selection we're representing
	XWindow m_owner;
	Condition m_result_cond;
	bool m_result_arrived = false;
	XAtom m_result_prop;
};

} // end ns

#endif // inc. guard

