#ifndef XWMFS_ENTRY_HXX
#define XWMFS_ENTRY_HXX

// C++
#include <atomic>

// xwmfs
#include "common/Helper.hxx"

// fwd. declaration
struct stat;

namespace xwmfs
{

// fwd. declarations
struct DirEntry;
struct FileEntry;
class OpenContext;

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

	//! unref()s parent entry if necessary
	virtual ~Entry();

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

	/**
	 * \brief
	 * 	Read data from the file
	 * \details
	 * 	A request to read data from the file object into the given
	 * 	buf of size bytes size, starting at the relative offset.
	 *
	 * 	The integer return value is the negative errno in case of
	 * 	error, or the number of bytes read on success.
	 **/
	virtual int read(OpenContext *ctx, char *buf, size_t size, off_t offset) = 0;

	/**
	 * \brief
	 * 	Write data to the file
	 * \details
	 * 	A request to write data from the given buf of size bytes size
	 * 	into the file object, starting at the relative offset.
	 *
	 * 	The integer return value is the negative errno in case of
	 * 	error, or the number of bytes written on success
	 **/
	virtual int write(OpenContext *ctx, const char *buf, size_t size, off_t offset) = 0;

	/**
	 * \brief
	 * 	Sets the parent directory for this entry
	 **/
	void setParent(DirEntry *dir);

	/**
	 * \brief
	 * 	Returns whether a file operation is currenctly allowed
	 * \return
	 * 	zero if it is allowed, otherwise a FUSE error code to return
	 * 	for any file operations
	 **/
	virtual int isOperationAllowed() const;

	/**
	 * \brief
	 * 	Creates a new file open context for this entry
	 * \details
	 * 	This function is called from the low level fuse functions to
	 * 	create a new file open context at file open time. This context
	 * 	uniquely identifies any open file instance.
	 *
	 * 	Derived classes can override this function and
	 * 	destroyOpenContext() to return types derived from OpenContext
	 * 	that contain additional data. They need to handle
	 * 	ref()/unref() then, however.
	 **/
	virtual OpenContext* createOpenContext();

	/**
	 * \brief
	 * 	Destroys an open context for this entry
	 * \details
	 * 	This function is called from the low level fuse functions to
	 * 	destroy an open context at file close time. This context was
	 * 	previously returned from createOpenContext().
	 **/
	virtual void destroyOpenContext(OpenContext *ctx);

	/**
	 * \brief
	 * 	Returns whether this type of file system entry requires fuse
	 * 	direct_io behaviour
	 * \details
	 * 	By default fuse does not implement direct_io behavour. This
	 * 	means the kernel will cache file contents and make some
	 * 	assumptions. userspace read/write calls will not be directly
	 * 	mapped to fuse calls into the file system.
	 *
	 * 	In some cases this is not what we want, for example when we
	 * 	don't know the file size in advance. See EventFile for an
	 * 	example.
	 **/
	virtual bool enableDirectIO() const { return false; }

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
		m_modify_time(time), m_status_time(time),
		m_refcount(1)
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
	std::atomic_size_t m_refcount;

	//! pointer to the parent directory
	DirEntry *m_parent = nullptr;
};

} // end ns

#endif // inc. guard

