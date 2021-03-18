#ifndef XWMFS_ROOT_ENTRY_HXX
#define XWMFS_ROOT_ENTRY_HXX

// C++

// xwmfs
#include "fuse/DirEntry.hxx"
#include "common/RWLock.hxx"

namespace xwmfs
{

/**
 * \brief
 * 	A specialized DirEntry that represents the file system root
 * \details
 * 	The RootEntry defines operations atop DirEntry that are of interest
 * 	for the file system on a global scale. The operations are:
 *
 * 	- looking up entries in the file system, recursively (this operation
 * 	could actually also be part of the DirEntry itself?)
 * 	- a file system global read-write lock for safe access to its
 * 	structure. For XWMFS purposes this is good enough. For a large file
 * 	system we'd need a more fine-grained approach
 *
 * 	The RootEntry does not really have a name. But we name it '/', as it
 * 	is conventional.
 **/
struct RootEntry :
	public DirEntry
{
public: // functions

	/**
	 * \brief
	 * 	Creates a new file system root with the given time value
	 **/
	RootEntry(const time_t &t = 0) :
		DirEntry("/", t)
	{
		//! root is his own parent
		this->setParent(this);
	}

	//! a wrapper for findEntry(const char*) using std::string
	Entry* findEntry(const std::string &path)
	{ return findEntry(path.c_str()); }

	/**
	 * \brief
	 * 	Lookup the given path, recursively, in the filesystem
	 * \param[in] path
	 * 	\c path is expected to be an absolute path within our
	 * 	filesystem.  The terminating entry may be of any type. The
	 * 	corresponding path must exist for success.
	 * \return
	 *	Corresponding entry or nullptr if not found
	 **/
	Entry* findEntry(const char *path);

	/**
	 * \brief
	 * 	Obtains a read-lock for the complete file system
	 * \details
	 * 	While the read lock is held the structure of the file system
	 * 	can't change. It can be obtained in parallel by multiple
	 * 	threads.
	 **/
	void readlock() const { m_lock.readlock(); }

	/**
	 * \brief
	 * 	Obtains a write-lock for the complete file system
	 * \details
	 * 	This lock can only be obtained by one exclusive thread. It
	 * 	allows alteration of the file system structure.
	 **/
	void writelock() { m_lock.writelock(); }

	/**
	 * \brief
	 * 	Returns a previously obtained read- or write-lock
	 **/
	void unlock() const { m_lock.unlock(); }

protected: // data

	//! the read-write lock protecting the file system
	xwmfs::RWLock m_lock;
};

} // end ns

#endif // inc. guard
