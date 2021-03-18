#ifndef XWMFS_SELECTIONOWNERFILE_HXX
#define XWMFS_SELECTIONOWNERFILE_HXX

#include "fuse/FileEntry.hxx"

namespace xwmfs
{

class SelectionDirEntry;

/**
 * \brief
 * 	Holds information about the owner windows of the various X selections
 * \details
 * 	The selection buffers in X are actually handled by the clients, the
 * 	server is just a broker to help clients find the communication partner
 * 	needed.
 *
 * 	Therefore each X selection has an owner. This is the window that
 * 	currently is responsible for providing the content of a given
 * 	selection buffer.
 *
 * 	This file here returns one line per supported selection buffer in the
 * 	form "<selection>: <window-id>".
 **/
class SelectionOwnerFile :
	public FileEntry
{
public: // functions

	SelectionOwnerFile(const std::string &n, const SelectionDirEntry &parent);

	int read(OpenContext *ctx, char *buf, size_t size, off_t offset) override;

protected: // functions

	void updateOwners();

protected: // data

	const SelectionDirEntry &m_selection_dir;
};

} // end ns

#endif // inc. guard
