#pragma once

// C++
#include <sstream>

// xwmfs
#include "common/types.hxx"
#include "fuse/Entry.hxx"

namespace xwmfs {

/*
 * I found it would be best to cache the information from the many X-related
 * objects in a format that is defined by the FUSE part of the code. The main
 * reason is that we need to be able to quickly traverse the virtual file
 * system such that we can e.g. lookup specific paths and list their contents.
 *
 * This would become costly if for each lookup the information needed first to
 * be gathered from complex objects and requiring a lot of checks etc.
 *
 * Thus we compile the information once into a ready-to-use data structure
 * that is optimized for FUSE needs.
 *
 * This of course means that we need to do more work when the information from
 * the window manager changes. Here we can restrict ourselves to incremental
 * updates of the FUSE data structures and thus don't need to re-translate the
 * whole file system again.
 */

/// This type represents regular file entries in the file system
/**
 * The FileEntry inherits from std::stringstream which allows us to easily
 * store and retrieve small bits of data from regular files. This makes
 * coding easy and should be enough for the purposes of XWMFS. We don't
 * intend to store huge files. And our files are always kept in RAM anyways.
 * 
 * FileEntry objects can be read-only or read-write. They should be read-write
 * when writing to it is possible and has a sensible effect on whatever it
 * represents.
 * 
 * The data to be returned on read is always considered to be present in
 * the inherited stringstream. Write calls, however, need to be handled
 * via specializations of FileEntry that overwrite the write-function
 * accordingly to do something sensible.
 **/
class FileEntry :
		public Entry,
		public std::stringstream {
public:
	/// Create a new FileEntry named `n`.
	/**
	 * \param[in] writable Whether the file is supposed to support writes.
	 * \param[in] the initial timestamp of the file
	 **/
	FileEntry(const std::string &n,
			const Writable writable = Writable{false},
			const time_t t = 0) :
			Entry{n, REG_FILE, t, writable} {
	}

	/// Base implementation of the FUSE write function.
	/**
	 * \return
	 *	Returns EINVAL to indicate "unsuitable object for writing".
	 * \note
	 *	This function should never be called directly. Objects that
	 *	aren't writable should be covered at open() time with EACCES.
	 *	Writable files should overwrite this function appropriately
	 *	instead.
	 *
	 *	If it is still called then "EINVAL" will be returned.
	 **/
	int write(OpenContext *ctx, const char *data, size_t size, off_t offset) override;

	int read(OpenContext *ctx, char *buf, size_t size, off_t offset) override;

	void getStat(struct stat*) const override;
};

} // end ns
