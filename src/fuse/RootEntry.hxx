#pragma once

// cosmos
#include <cosmos/thread/RWLock.hxx>

// xwmfs
#include "fuse/DirEntry.hxx"

namespace xwmfs {

/// A specialized DirEntry that represents the file system root.
/**
 * The RootEntry defines operations atop DirEntry that are of interest for the
 * file system on a global scale. The operations are:
 * 
 * - looking up entries in the file system, recursively (this operation could
 *   actually also be part of the DirEntry itself?)
 * - a file system global read-write lock for safe access to its
 *   structure. For XWMFS purposes this is good enough. For a large file
 *   system we'd need a more fine-grained approach
 * 
 * The RootEntry does not really have a name. But we name it '/', as it
 * is conventional.
 **/
struct RootEntry :
		public DirEntry {
public: // functions

	/// Creates a new file system root with the given time value.
	RootEntry(const time_t t = 0) :
			DirEntry{"/", t} {
		/// root is its own parent
		this->setParent(this);
	}

	/// Lookup the given path recursively in the filesystem.
	/**
	 * \param[in] path
	 * 	This is expected to be an absolute path within the filesystem.
	 * 	The terminating entry may be of any type. The corresponding
	 * 	path must exist for success.
	 * \return
	 *	Corresponding entry or nullptr if not found.
	 **/
	Entry* findEntry(const std::string_view path);

	/// Obtains a read-lock for the complete file system.
	/**
	 * While the read lock is held the structure of the file system can't
	 * change. It can be obtained in parallel by multiple threads.
	 **/
	void readlock() const { m_lock.readlock(); }

	/// Obtains a write-lock for the complete file system.
	/**
	 * This lock can only be obtained by one thread exclusively. It allows
	 * alteration of the file system structure.
	 **/
	void writelock() { m_lock.writelock(); }

	/// Returns a previously obtained read- or write-lock.
	void unlock() const { m_lock.unlock(); }

protected: // data

	/// The read-write lock protecting the file system
	cosmos::RWLock m_lock;
};

} // end ns
