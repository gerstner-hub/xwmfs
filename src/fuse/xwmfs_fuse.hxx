#ifndef XWMFS_FUSE_HXX
#define XWMFS_FUSE_HXX

// C++
#include <cstring>
#include <sstream>
#include <map>

// xwmfs
#include "common/RWLock.hxx"

/*
 * This header defines the data structures for the actual file system
 * information
 * 
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
 * the window manager changes. Here we can hopefully restrict ourselves to
 * incremental updates of the fuse data structures and thus don't need to
 * re-translate the whole file system again.
 */

// fwd. declaration
struct stat;

namespace xwmfs
{

//! for comparison in maps with char pointers as keys
struct compare_cstring
{
	bool operator()(const char *a, const char *b) const
	{ return std::strcmp(a, b) < 0; }
};

// fwd. declarations
struct DirEntry;
struct FileEntry;

/**
 * \brief
 * 	Base class for file system entries
 * \details
 * 	The file system tree will consist of instances of this base type. I
 * 	use an enumeration for differentiation of specific types to avoid too
 * 	high performance penalties due to RTTI.
 **/
struct Entry
{
public: // types

	/**
	 * \brief
	 * 	Possible specializations of file system entries
	 **/
	enum Type
	{
		DIRECTORY,
		REG_FILE,
		INVAL_TYPE
	};

public: // functions

	// make sure entries are never flat-copied
	Entry(const Entry &other) = delete;

	//! makes sure derived class's destructors are always called
	virtual ~Entry() { }

	/**
	 * \brief
	 * 	casts entry to a DirEntry, if the type matches
	 * \return
	 * 	A DirEntry pointer to \c entry or nullptr if the entry is no
	 * 	DirEntry or nullptr itself
	 **/
	static DirEntry* tryCastDirEntry(Entry* entry)
	{
		if( entry && entry->isDir() )
			return reinterpret_cast<DirEntry*>(entry);

		return nullptr;
	}

	/**
	 * \brief
	 * 	casts entry to a FileEntry, if the type matches
	 * \return
	 * 	A FileEntry pointer to \c entry or nullptr if the entry is no
	 * 	FileEntry or nullptr itself
	 **/
	static FileEntry* tryCastFileEntry(Entry *entry)
	{
		if( entry && entry->isRegular() )
			return reinterpret_cast<FileEntry*>(entry);

		return nullptr;
	}

	//! returns the name of the file system entry
	const std::string& name() const { return m_name; }

	//! returns the type of the file system entry
	Type type() const { return m_type; }

	//! returns whether this entry is of the DIRECTORY type
	bool isDir() const { return type() == DIRECTORY; }

	//! returns whether this entry is of the REG_FILE type
	bool isRegular() const { return type() == REG_FILE; }

	//! returns whether the file system entry is writable
	bool isWritable() const { return m_writable; }

	//! sets the modification time of the file system entry to \c t
	void setModifyTime(const time_t &t) { m_modify_time = t; }

	//! sets the status time of the file system entry to \c t
	void setStatusTime(const time_t &t) { m_status_time = t; }

	//! gets the current modification time of the file system entry
	const time_t& getModifyTime() const { return m_modify_time; }

	//! gets the current status time of the file system entry
	const time_t& getStatusTime() const { return m_status_time; }

	/**
	 * \brief
	 * 	Fills in the status information corresponding to this entry
	 * 	into \c s
	 **/
	virtual void getStat(struct stat *s);

	//! increases the reference count
	void ref() { m_refcount++; }
	//! decreases the reference count and returns whether the entry must
	//! be deleted
	bool unref() { return --m_refcount == 0; }
	//! marks the entry for deletion and unreferences it, returns unref()
	virtual bool markDeleted() { m_deleted = true; return unref(); }
	//! returns whether this entry is pending deletion
	bool isDeleted() const { return m_deleted; }

protected: // functions

	/**
	 * \brief
	 * 	Constructs a new file system entry
	 * \details
	 * 	This constructor is protected as a file system entry should
	 * 	only be constructed as a specialized object like DirEntry or
	 * 	FileEntry.
	 *
	 * 	The file system entry will get the name \c n, the type \c t,
	 * 	will be handled as writable if \c writable is set and the
	 * 	initial status and modification times will be \c time.
	 **/
	Entry(
		const std::string &n,
		const Type &t,
		const bool writable = false,
		const time_t &time = 0
	) :
		m_name(n), m_type(t), m_writable(writable),
		m_modify_time(time), m_status_time(time)
	{ };

	/**
	 * \brief
	 * 	Converts untrusted input \c data of \c bytes length into its
	 * 	integer representation
	 * \details
	 * 	The string in \c data can be octal, hexadecimal or decimal
	 * 	using the typical syntaxes.
	 * \return
	 * 	< 0 if an error occured. This will then be the error code to
	 * 	return to FUSE. >= 0 is an integer could be parsed. This will
	 * 	then be the number of characters from \c data that have been
	 * 	parsed.
	 **/
	int parseInteger(const char *data, const size_t bytes, int &result) const;

protected: // data
	
	const std::string m_name;
	const Type m_type = INVAL_TYPE;
	const bool m_writable = false;

	//! set to the last write/creation event
	time_t m_modify_time = 0;
	//! set to the creation time, metadata isn't changed afterwards any
	//! more
	time_t m_status_time = 0;

	//! the user id we're running as
	static const uid_t m_uid;
	//! the group id we're running as
	static const gid_t m_gid;

	//! whether the file system entry was removed and is pending deletion
	bool m_deleted = false;
	/**
	 * \brief
	 * 	Reference count of the file system entry
	 * \details
	 * 	This counter is one upon construction and is increased for
	 * 	each open file description on the FUSE side, decreased again
	 * 	for each closed file description.
	 *
	 * 	Entries a typically not removed from the FUSE side but from
	 * 	the X11 side, because a Window disappears or alike. In this
	 * 	case the initial single reference is decremented. Whoever
	 * 	drops the count to zero needs to delete the entry memory.
	 *
	 * 	This complex handling is necessary, because multiple clients
	 * 	can open a file at the same time and also, because the
	 * 	removals originate from external events in the X server. When
	 * 	somebody still has a file opened then the data structure needs
	 * 	to remain valid until all file descriptors to it have been
	 * 	closed.
	 **/
	size_t m_refcount = 1;
};

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
	virtual int write(const char *data, const size_t bytes, off_t offset)
	{
		(void)data;
		(void)bytes;
		(void)offset;
		return -EINVAL;
	}

	void getStat(struct stat*) override;
};


/**
 * \brief
 * 	This type represents directory entries in the file system
 * \details
 * 	The peculiarity of a directory is, of course, that it contains other
 * 	file system entries. For quick access a DirEntry thus contains a map
 * 	container that maps from its contained names to the corresponding
 * 	Entry base type.
 *
 * 	For now DirEntries are always read-only as we can't create new files
 * 	in the XWMFS (yet).
 **/
struct DirEntry :
	public Entry
{
public: // types

	//! \brief
	//! a map type that maps file system names to their corresponding
	//! objects
	typedef std::map<const char*, Entry*, compare_cstring> NameEntryMap;

	//! The type enum associated with DirEntry. Can be used in templates.
	static const Entry::Type type = Entry::DIRECTORY;

public: // functions

	/**
	 * \brief
	 * 	Constructs a new directory of name \c n
	 * \details
	 * 	The given time \c t will be used for the initial status and
	 * 	modification times of the directory.
	 **/
	DirEntry(const std::string &n, const time_t &t = 0) :
		Entry(n, DIRECTORY, false, t)
	{ }

	/**
	 * \brief
	 * 	When a DirEntry is destroyed it deletes all its contained
	 * 	objects
	 * \details
	 * 	Due to this behaviour the file system can recursively delete
	 * 	itself. As our file system won't get very deep nest levels we
	 * 	don't need to fear a stack overflow.
	 **/
	~DirEntry() { this->clear(); }

	/**
	 * \brief
	 * 	Removes all contained file system objects and marks them for
	 * 	deletion
	 **/
	void clear();
	
	/**
	 * \brief
	 * 	Template wrapper around addEntry(Entry*, const bool) that
	 * 	returns the concrete type added
	 * \details
	 * 	This template allows us to return the exact type added from
	 * 	the function call. This way some statements are written more
	 * 	easily (e.g. creating a new directory entry directly like
	 * 	this:
	 *
	 * 	DirEntry *new_subdir = dir->addEntry( new DirEntry(...) )
	 **/
	template <typename ENTRY>
	ENTRY* addEntry(ENTRY * const e, const bool inherit_time = true)
	{
		addEntry(static_cast<Entry*>(e), inherit_time);
		return e;
	}

	/**
	 * \brief
	 * 	Adds an arbitrary entry to the current directory
	 * \details
	 * 	The given file system entry \c e will be added as member of
	 * 	the current DirEntry object. If \c inherit_time is set then
	 * 	the modification and status time of \c e will be set to the
	 * 	respective values of the current DirEntry object.
	 **/
	Entry* addEntry(Entry * const e, const bool inherit_time = true);

	//! wrapper for getEntry(const char*) using a std::string
	Entry* getEntry(const std::string &s) const
	{ return getEntry(s.c_str()); }

	/**
	 * \brief
	 * 	Retrieve the contained entry with the name \c n
	 * \return
	 * 	A pointer to the contained entry or nullptr if there is no
	 * 	entry with that name contained in the current DirEntry.
	 **/
	Entry* getEntry(const char *n) const
	{
		auto obj_it = m_objs.find(n);

		return ( obj_it == m_objs.end() ) ? nullptr : obj_it->second;
	}
	
	//! retrieve an entry in the directory with name \c n, but only if of
	//! type \c t
	Entry* getEntry(const char *n, const Entry::Type &t)
	{
		Entry *ret = this->getEntry(n);
		if( ret && ret->type() != t )
			return nullptr;

		return ret;
	}

	DirEntry* getDirEntry(const char *n)
	{
		return reinterpret_cast<DirEntry*>(getEntry(n, Entry::Type::DIRECTORY));
	}

	FileEntry* getFileEntry(const char *n)
	{
		return reinterpret_cast<FileEntry*>(getEntry(n, Entry::Type::REG_FILE));
	}

	//! wrapper for removeEntry(const char*) using a std::string
	void removeEntry(const std::string &s)
	{ return removeEntry(s.c_str()); }

	/**
	 * \brief
	 * 	Removes the contained entry with the name \c s
	 * \details
	 * 	Throws an Exception if no such entry is existing
	 **/
	void removeEntry(const char* s);

	//! Retrieves the constant map of all contained entries
	const NameEntryMap& getEntries() const { return m_objs; }

	bool markDeleted() override
	{
		// make sure all child entries get marked as deleted right
		// away as well
		clear();
		return Entry::markDeleted();
	}

protected: // data

	//! contains all objects that the directory contains with their names
	//! as keys
	NameEntryMap m_objs;
};

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
	{ }

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

/**
 * \brief
 * 	A scope-guard object for read-locking a complete file system
 **/
class FileSysReadGuard
{
public:
	FileSysReadGuard( const RootEntry &root ) : m_root(root)
	{
		root.readlock();
	}

	~FileSysReadGuard() { m_root.unlock(); }
private:
	const RootEntry &m_root;
};

/**
 * \brief
 * 	A scope-guard object for write-locking a complete file system
 **/
class FileSysWriteGuard
{
public:
	FileSysWriteGuard( RootEntry &root ) : m_root(root)
	{
		root.writelock();
	}

	~FileSysWriteGuard() { m_root.unlock(); }
private:
	RootEntry &m_root;
};

} // end ns

#endif // inc. guard

