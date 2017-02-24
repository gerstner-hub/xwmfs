#ifndef XWMFS_FILE_ENTRY_HXX
#define XWMFS_FILE_ENTRY_HXX

// C++
#include <sstream>

// xwmfs
#include "fuse/Entry.hxx"

namespace xwmfs
{

/*
 * I found it would be best to cache the information from the many X-related
 * objects in a format that is defined by the fuse part of the code. The main
 * reason is that we need to be able to quickly traverse the virtual file
 * system such that we can e.g. lookup specific paths and list their contents.
 * 
 * This would become costly if for each lookup the information needed first to
 * be gathered from complex objects and requiring a lot of checks etc.
 * 
 * Thus we compile the information once into a ready-to-use data structure
 * that is optimized for fuse needs.
 *
 * This of course means that we need to do more work when the information from
 * the window manager changes. Here we can restrict ourselves to incremental
 * updates of the fuse data structures and thus don't need to re-translate the
 * whole file system again.
 */

/**
 * \brief
 * 	This type represents regular file entries in the file system
 * \details
 * 	The FileEntry inherits from std::stringstream which allows us to
 * 	easily store and retrieve small bits of data from our regular files.
 * 	This makes coding easy and should be enough for the purposes of XWMFS.
 * 	We don't intend to store huge files. And our files are always kept in
 * 	RAM anyways.
 *
 * 	FileEntries can be read-only or read-write. They should be read-write
 * 	when writing to it is possible and has a sensible effect on whatever
 * 	it represents.
 *
 * 	The data to be returned on read is always considered to be present in
 * 	the inherited stringstream. Write calls, however, need to be handled
 * 	via specializations of FileEntry that overwrite the write-function
 * 	accordingly to do something sensible.
 **/
struct FileEntry :
	public Entry,
	public std::stringstream
{
	/**
	 * \brief
	 * 	Create a new FileEntry with name \c n, being read-write if \c
	 * 	writable is set and using \c t for initial timestamps
	 **/
	FileEntry(
		const std::string &n,
		const bool writable = false,
		const time_t &t = 0
	) : Entry(n, REG_FILE, writable, t) { }

	/**
	 * \brief
	 *	Base implementation of write function.
	 * \return
	 *	Returns EINVAL to indicate "unsuitable object for writing"
	 * \note
	 *	Calling this function should never happen. Objects that aren't
	 *	writable should be covered at open() time with EACCES.
	 *	Writable files should overwrite this function appropriately.
	 *
	 *	If it is still called then "invalid argument" will be
	 *	returned.
	 **/
	int write(const char *data, size_t size, off_t offset) override;

	int read(char *buf, size_t size, off_t offset) override;

	void getStat(struct stat*) override;
};

} // end ns

#endif // inc. guard

