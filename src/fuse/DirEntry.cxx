// xwmfs
#include "fuse/DirEntry.hxx"
#include "common/Exception.hxx"

// C++
#include <cassert>
#include <sstream>

namespace xwmfs
{

Entry* DirEntry::addEntry(Entry * const e, const bool inherit_time)
{
	assert(e);

	/*
	 * we optimize here by not allocating a copy of the entry's
	 * name but instead use a flat copy of that entries name as a
	 * key. we need to be very careful about that, however, when
	 * it comes to deleting entries again.
	 */
	auto insert_res = m_objs.insert( std::make_pair(e->name().c_str(), e) );

	if( ! insert_res.second )
	{
		xwmfs_throw( Exception(
			std::string("double-add of the same directory node \"") + e->name() + "\""
		));
	}

	// we inherit our own time info to the new entry, if none has been
	// specified
	if( inherit_time )
	{
		e->setModifyTime(m_modify_time);
		e->setStatusTime(m_status_time);
	}

	e->setParent( this );

	return e;
}

void DirEntry::removeEntry(const char* s)
{
	auto it = m_objs.find(s);

	if( it == m_objs.end() )
	{
		std::stringstream ss;
		ss << "removeEntry: No such entry \"" << s << "\"";
		xwmfs_throw( Exception(ss.str()) );
	}

	auto entry = it->second;

	m_objs.erase(it);

	if( entry->markDeleted() )
	{
		// only delete the entry after erasing it from the map,
		// because the string from the entry is used a key for the map
		// (flat copy)
		delete entry;
	}
}

void DirEntry::clear()
{
	std::vector<Entry*> to_delete;

	// we need to be careful here as our keys are kept in the
	// mapped values. If we delete an entry then its key in the map
	// becomes invalid.

	for( auto it: m_objs )
	{
		auto entry = it.second;
		if( entry->markDeleted() )
		{
			to_delete.push_back(entry);
		}
	}

	m_objs.clear();

	for( auto &entry: to_delete )
	{
		delete entry;
	}
}

int DirEntry::read(char *buf, const size_t size, off_t offset)
{
	(void)buf;
	(void)size;
	(void)offset;
	// reading a directory via read doesn't make much sense
	return -EISDIR;
}

int DirEntry::write(const char *buf, const size_t size, off_t offset)
{
	(void)buf;
	(void)size;
	(void)offset;
	return -EISDIR;
}

} // end ns

