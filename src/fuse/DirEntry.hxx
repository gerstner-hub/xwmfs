#ifndef XWMFS_DIR_ENTRY_HXX
#define XWMFS_DIR_ENTRY_HXX

// C++
#include <map>

// xwmfs
#include "fuse/Entry.hxx"

namespace xwmfs
{

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

} // end ns

#endif // inc. guard

