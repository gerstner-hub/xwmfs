#pragma once

// C++
#include <map>

// cosmos
#include <cosmos/string.hxx>
#include <cosmos/thread/Mutex.hxx>
#include <cosmos/utils.hxx>

// xwmfs
#include "fuse/Entry.hxx"
#include "main/Exception.hxx"

namespace xwmfs {

/// This type represents directory entries in the file system.
/**
 * The peculiarity of a directory is, of course, that it contains other file
 * system entries. For quick access a DirEntry thus contains a map container
 * that maps from its contained names to the corresponding Entry base type.
 *
 * For now DirEntry object are always read-only as we can't create new files
 * in the XWMFS (yet).
 **/
class DirEntry :
		public Entry {
public: // types

	/// A map of file system names to their corresponding Entry objects.
	using NameEntryMap = std::map<const char*, Entry*, cosmos::CompareCString>;

	/// The type enum associated with DirEntry. Can be used in templates.
	static constexpr Entry::Type type = Entry::DIRECTORY;

	struct DoubleAddError :
			public Exception {
		explicit DoubleAddError(const std::string &name,
				const cosmos::SourceLocation &src_loc = cosmos::SourceLocation::current()) :
			Exception{
				std::string{"double-add of the same directory node \""} + name.c_str() + "\"",
				src_loc
			}
		{}
	};

	using InheritTime = cosmos::NamedBool<struct inherit_time_t, true>;

public: // functions

	/// Constructs a new directory of name `n`.
	/**
	 * The given time `t` will be used for the initial status and
	 * modification times of the directory.
	 **/
	DirEntry(const std::string &n, const time_t &t = 0) :
		Entry{n, DIRECTORY, t} {
	}

	/// When a DirEntry is destroyed it deletes all its contained objects.
	/**
	 * Due to this behaviour the file system can recursively delete
	 * itself. As our file system won't get very deep nesting levels we
	 * don't need to fear a stack overflow.
	 **/
	~DirEntry() { this->clear(); }

	/// Removes all contained file system objects and marks them for deletion.
	void clear();

	/// Template wrapper around addEntry(Entry*, const InheritTime) that returns the concrete type added.
	/**
	 * This template allows us to return the exact type added from the
	 * function call. This way some statements are written more easily,
	 * e.g. creating a new directory entry directly like this:
	 * 
	 * ```
	 * DirEntry *new_subdir = dir->addEntry(new DirEntry{...})
	 * ```
	 **/
	template <typename ENTRY>
	ENTRY* addEntry(ENTRY * const e, const InheritTime inherit_time = InheritTime{true}) {
		addEntry(static_cast<Entry*>(e), inherit_time);
		return e;
	}

	/// Adds a new entry to the current directory.
	/**
	 * The given file system entry `e` will be added as member of the
	 * current DirEntry object. If `inherit_time` is set then the
	 * modification and status time of `e` will be set to the respective
	 * values of the current DirEntry object.
	 **/
	Entry* addEntry(Entry * const e, const InheritTime inherit_time = InheritTime{true});

	/// Wrapper for `getEntry(const char*)` using a std::string
	Entry* getEntry(const std::string &s) const {
		return getEntry(s.c_str());
	}

	/// Retrieve the contained entry with the name `n`.
	/**
	 * \return
	 * 	A pointer to the contained Entry or nullptr if there is no
	 * 	entry with that name contained in the current DirEntry.
	 **/
	Entry* getEntry(const char *n) const {
		auto obj_it = m_objs.find(n);

		return (obj_it == m_objs.end()) ? nullptr : obj_it->second;
	}

	/// Retrieve an entry in the directory with name `n` and of type `t`.
	/**
	 * If an entry of the given name exists, but has a different type,
	 * then nullptr is returned.
	 **/
	Entry* getEntry(const char *n, const Entry::Type t) {
		Entry *ret = this->getEntry(n);
		if (ret && ret->type() != t)
			return nullptr;

		return ret;
	}

	DirEntry* getDirEntry(const char *n) {
		return reinterpret_cast<DirEntry*>(getEntry(n, Entry::Type::DIRECTORY));
	}

	DirEntry* getDirEntry(const std::string &n) {
		return getDirEntry(n.c_str());
	}

	FileEntry* getFileEntry(const char *n) {
		return reinterpret_cast<FileEntry*>(getEntry(n, Entry::Type::REG_FILE));
	}

	FileEntry* getFileEntry(const std::string &n) {
		return getFileEntry(n.c_str());
	}

	/// wrapper for removeEntry(const char*) using a std::string.
	void removeEntry(const std::string &s) {
		return removeEntry(s.c_str());
	}

	/// Removes the contained entry with the name `s`
	/**
	 * Throws an Exception if no such entry is existing.
	 **/
	void removeEntry(const char *s);

	/// Retrieves the non-modifiable map of all contained entries.
	const NameEntryMap& getEntries() const {
		return m_objs;
	}

	bool markDeleted() override {
		// make sure all child entries get marked as deleted right
		// away as well
		clear();
		return Entry::markDeleted();
	}

	int read(OpenContext *ctx, char *buf, size_t size, off_t offset) override;
	int write(OpenContext *ctx, const char *buf, size_t size, off_t offset) override;

	cosmos::Mutex& getLock() {
		return m_lock;
	}

protected: // data

	/// Contains all entries existing in the directory with their names as keys.
	NameEntryMap m_objs;

	/// A lock for this directory and all its direct children.
	/**
	 * For serializing parallel access to individual fs entries we need
	 * some kind of locking strategy. A global lock would unnecessarily
	 * serialize unrelated operations on different files. One lock per
	 * entry seems waste of resources if we have many files. Thus I've
	 * settled for one lock per directory and all its direct non-directory
	 * children. A middle way of saving resources and still allowing
	 * parallel operations in many situations.
	 **/
	cosmos::Mutex m_lock;
};

} // end ns
