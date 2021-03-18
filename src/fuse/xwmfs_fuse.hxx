#ifndef XWMFS_FUSE_HXX
#define XWMFS_FUSE_HXX

// xwmfs
#include "fuse/RootEntry.hxx"

/*
 * This header defines miscellaneous data structures for the fuse part of
 * xwmfs
 */

namespace xwmfs
{

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
 * 	A scope-guard object for temporarily releasing a real-lock of the
 * 	complete file system
 **/
class FileSysRevReadGuard
{
public:
	FileSysRevReadGuard( const RootEntry &root ) : m_root(root)
	{
		root.unlock();
	}

	~FileSysRevReadGuard() { m_root.readlock(); }
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
