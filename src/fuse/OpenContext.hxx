#ifndef XWMFS_OPEN_CONTEXT_HXX
#define XWMFS_OPEN_CONTEXT_HXX

// xwmfs

namespace xwmfs
{

class Entry;

class OpenContext
{
public: // functions

	OpenContext(Entry *entry) :
		m_entry(entry)
	{}

	virtual ~OpenContext() {}

	Entry* getEntry() { return m_entry; }

protected: // data

	//! the file entry that has been opened
	Entry *m_entry;
};

} // end ns

#endif // inc. guard

