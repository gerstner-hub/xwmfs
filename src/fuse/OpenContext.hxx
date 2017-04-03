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

	bool isNonBlocking() const { return m_nonblocking; }
	void setNonBlocking(const bool nb) { m_nonblocking = nb; }

protected: // data

	//! the file entry that has been opened
	Entry *m_entry;
	//! whether the file descriptor is in non-blocking mode
	bool m_nonblocking = false;
};

} // end ns

#endif // inc. guard

