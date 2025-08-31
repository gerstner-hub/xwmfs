#pragma once

// C++
#include <atomic>
#include <memory>

// cosmos
#include <cosmos/thread/pthread.hxx>
#include <cosmos/fwd.hxx>

// POSIX
#include <unistd.h>

// xwmfs
#include <common/types.hxx>
#include <fuse/AbortHandler.hxx>

// forward declarations
struct stat;

namespace xwmfs {

class DirEntry;
class FileEntry;
class OpenContext;

/// Base class for file system entries.
/**
 * The file system tree will consist of instances of this base type. An
 * enumeration is used for differentiation of specific types to avoid too high
 * performance penalties due to RTTI.
 **/
class Entry {
public: // types

	/// Possible specializations of file system entries.
	enum class Type {
		DIRECTORY,
		REG_FILE,
		SYMLINK,
		INVAL_TYPE
	};

	using enum Type;

public: // functions

	// make sure entries are never flat-copied
	Entry(const Entry &other) = delete;

	/// `unref()`s the parent entry, if necessary.
	virtual ~Entry();

	/// Casts `entry` to a DirEntry, if the type matches.
	/**
	 * \return
	 * 	A DirEntry pointer to `entry` or nullptr if the entry is no
	 * 	DirEntry or nullptr itself.
	 **/
	static DirEntry* tryCastDirEntry(Entry* entry) {
		if (entry && entry->isDir())
			return reinterpret_cast<DirEntry*>(entry);

		return nullptr;
	}

	/// Casts `entry` to a FileEntry, if the type matches
	/**
	 * \return
	 * 	A FileEntry pointer to `entry` or nullptr if the entry is no
	 * 	FileEntry or nullptr itself.
	 **/
	static FileEntry* tryCastFileEntry(Entry *entry) {
		if (entry && entry->isRegular())
			return reinterpret_cast<FileEntry*>(entry);

		return nullptr;
	}

	/// Returns the name of the file system entry.
	const std::string& name() const { return m_name; }

	/// Returns the type of the file system entry.
	Type type() const { return m_type; }

	/// Returns whether this entry is of the DIRECTORY type.
	bool isDir() const { return type() == DIRECTORY; }

	/// Returns whether this entry is of the REG_FILE type.
	bool isRegular() const { return type() == REG_FILE; }

	/// Returns whether this entry is of the SYMLINK type.
	bool isSymlink() const { return type() == SYMLINK; }

	/// Returns whether the file system entry is writable.
	bool isWritable() const { return m_writable; }

	/// Sets the modification time of the file system entry to `t`.
	void setModifyTime(const time_t &t) { m_modify_time = t; }

	/// Sets the status time of the file system entry to `t`.
	void setStatusTime(const time_t &t) { m_status_time = t; }

	/// Gets the current modification time of the file system entry.
	const time_t& getModifyTime() const { return m_modify_time; }

	/// Gets the current status time of the file system entry.
	const time_t& getStatusTime() const { return m_status_time; }

	/// Fills in status information corresponding to this entry into `s`.
	virtual void getStat(struct stat *s) const;

	/// Increases the node reference count
	void ref() { m_refcount++; }
	/// Decreases the node reference count and returns whether the entry must be deleted.
	bool unref() { return --m_refcount == 0; }
	/// Marks the entry for deletion and unreferences it, returns unref().
	virtual bool markDeleted() { m_deleted = true; return unref(); }
	/// Returns whether this entry is pending for deletion.
	bool isDeleted() const { return m_deleted; }

	/// Read data from the file.
	/**
	 * A request to read data from the file object into the given buf of
	 * size bytes size, starting at the relative `offset`.
	 * 
	 * The integer return value is the negative errno in case of error, or
	 * the number of bytes read on success.
	 **/
	virtual int read(OpenContext *ctx, char *buf, size_t size, off_t offset) {
		(void)ctx;
		(void)buf;
		(void)size;
		(void)offset;
		return -EINVAL;
	}

	/// Write data to the file.
	/**
	 * A request to write data from the given buf of size bytes size into
	 * the file object, starting at the relative offset.
	 * 
	 * The integer return value is the negative errno in case of error, or
	 * the number of bytes written on success.
	 **/
	virtual int write(OpenContext *ctx, const char *buf, size_t size, off_t offset) {
		(void)ctx;
		(void)buf;
		(void)size;
		(void)offset;
		return -EINVAL;
	}

	/// Read the link target from a symlink.
	/**
	 * `buf` should be terminated with a null byte. If the link target
	 * does not fit into the buffer then it should be truncated.
	 **/
	virtual int readlink(char *buf, size_t size) {
		(void)buf;
		(void)size;
		return -EINVAL;
	}

	/// Sets the parent directory for this entry.
	void setParent(DirEntry *dir);

	/// Returns whether a file operation is currently allowed.
	/**
	 * \return
	 * 	zero if it is allowed, otherwise a FUSE error code to return
	 * 	for any file operations.
	 **/
	virtual int isOperationAllowed() const;

	/// Creates a new file open context for this entry.
	/**
	 * This function is called from the low level FUSE functions to create
	 * a new file open context at file open time. This context uniquely
	 * identifies an open file instance.
	 * 
	 * Derived classes can override this function as well as
	 * destroyOpenContext() to return types derived from OpenContext that
	 * contain additional data. They need to handle ref()/unref() then,
	 * however.
	 **/
	virtual OpenContext* createOpenContext();

	/// Destroys an open context for this entry.
	/**
	 * This function is called from the low level FUSE functions to
	 * destroy an open context at file close time. This context was
	 * previously returned from createOpenContext().
	 **/
	virtual void destroyOpenContext(OpenContext *ctx);

	/// Returns whether this type of file system entry requires FUSE direct_io behaviour.
	/**
	 * By default FUSE does not implement direct_io behaviour. This means
	 * the kernel will cache file contents and make some assumptions.
	 * Userspace read/write calls will not be directly mapped to FUSE
	 * calls into the file system.
	 * 
	 * In some cases this is not what we want, for example when we
	 * don't know the file size in advance. See EventFile for an
	 * example.
	 **/
	virtual bool enableDirectIO() const { return false; }

	/// Abort an ongoing blocking read call, if any and if supported.
	void abortBlockingCall(const cosmos::pthread::ID thread);

protected: // functions

	void createAbortHandler(cosmos::Condition &cond);

	/// Constructs a new file system entry.
	/**
	 * This constructor is protected, as a file system entry should only
	 * be constructed as a specialized object like DirEntry or FileEntry.
	 * 
	 * The file system entry will get the name `n`, type `t`, will
	 * be handled as writable if `writable` is set and the initial status
	 * and modification times will be `time`.
	 **/
	Entry(const std::string &n, const Type &t,
			const time_t &time,
			const Writable writable = Writable{false}) :
		m_name{n}, m_type{t}, m_writable{writable},
		m_modify_time{time}, m_status_time{time},
		m_refcount{1} {
	};

	/// Converts untrusted input `data` of `bytes` length into its integer representation.
	/**
	 * The string in `data` can be octal, hexadecimal or decimal using
	 * the typical syntaxes.
	 *
	 * \return
	 * 	< 0 if an error occurred. This will then be the error code to
	 * 	return to FUSE. >= 0 if an integer could be parsed. This will
	 * 	then be the number of characters from `data` that have been
	 * 	parsed.
	 **/
	int parseInteger(const char *data, const size_t bytes, int &result) const;

protected: // data

	const std::string m_name;
	const Type m_type = INVAL_TYPE;
	const Writable m_writable;

	/// Time of the last write/creation event.
	time_t m_modify_time = 0;
	/// Time of creation, this isn't changed afterwards any more.
	time_t m_status_time = 0;

	/// The user id we're running as.
	static const uid_t m_uid;
	//! The group id we're running as.
	static const gid_t m_gid;

	//! Whether the file system entry was removed and is pending deletion.
	bool m_deleted = false;

	/// Reference count of the file system entry.
	/**
	 * This counter is 1 upon construction and is increased for each open
	 * file description on the FUSE side, decreased again for each closed
	 * file description.
	 * 
	 * Entries a typically not removed on FUSE request but from the X11
	 * side, because a Window disappears or alike. In this case the
	 * initial single reference is decremented. Whoever drops the count to
	 * zero needs to delete the entry from memory.
	 * 
	 * This complex handling is necessary, because multiple clients can
	 * open a file at the same time and also, because the removals
	 * originate from external events in the X server. When somebody still
	 * has a file opened then the data structure needs to remain valid
	 * until all file descriptors to it have been closed.
	 **/
	std::atomic_size_t m_refcount;

	/// Pointer to the parent directory
	DirEntry *m_parent = nullptr;

	/// Helper for dealing with blocking call abortion.
	std::unique_ptr<AbortHandler> m_abort_handler;
};

} // end ns
