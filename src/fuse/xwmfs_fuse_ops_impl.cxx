/*
 * This is the implementation of the fuse specific functions that make xwmfs
 * actually work as a FS.
 *
 * The file system operations are called directly by FUSE as soon as some
 * access to the file system occurs.
 */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "fuse/xwmfs_fuse.hxx"
#include "fuse/Entry.hxx"
#include "fuse/FileEntry.hxx"
#include "fuse/xwmfs_fuse_ops.h"
#include "main/Xwmfs.hxx"
#include "common/Exception.hxx"

namespace xwmfs
{

xwmfs::RootEntry *filesystem = nullptr;

}

/**
 * \brief
 * 	Get stat information about a file system entry
 * \details
 * 	stat is done w/o an open context
 **/
int xwmfs_getattr(const char *path, struct stat *stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));
	xwmfs::FileSysReadGuard read_guard( *xwmfs::filesystem );

	xwmfs::Entry *entry = xwmfs::filesystem->findEntry( path );

	if( ! entry )
	{
		xwmfs::StdLogger::getInstance().debug()
			<< __FUNCTION__ << ": noent for path "
			<< path << "\n";
		return -ENOENT;
	}

	xwmfs::StdLogger::getInstance().debug()
		<< __FUNCTION__ << ": stat for path "
		<< path << "\n";

	entry->getStat(stbuf);

	return 0;
}

/**
 * \brief
 * 	Request to list the contents of a directory
 * \details
 * 	The \c filler object allows for easy creation of the required dirent
 * 	structures behind the scene.
 **/
int xwmfs_readdir(
	const char *path, void *buf, fuse_fill_dir_t filler,
	 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	xwmfs::FileSysReadGuard read_guard( *xwmfs::filesystem );

	/*
	 * note: right now we always lookup the file system entry within
	 * readdir. We could optimize this by implementing opendir() and set a
	 * file handle there just like it is done in open(). Also opendir()
	 * should do access checking.
	 *
	 * However, we're currently always allowing access to directories for
	 * reading. Also this is the only relevant operation for directories
	 * we're currently implementing so the performance benefit from
	 * implement opendir() is small.
	 */
	xwmfs::Entry *entry = xwmfs::filesystem->findEntry( path );

	xwmfs::DirEntry *dir_entry = xwmfs::Entry::tryCastDirEntry(entry);

	if( ! entry )
	{ 
		xwmfs::StdLogger::getInstance().debug()
			<< __FUNCTION__ << ": no such entity: "
			<< path << "\n";

		return -ENOENT;
	}
	else if( ! dir_entry )
	{
		xwmfs::StdLogger::getInstance().debug()
			<< __FUNCTION__ << ": not a dir: "
			<< path << "\n";
		return -ENOTDIR;
	}

	// okay we found a valid directory to list the contents of
	const auto &entries = dir_entry->getEntries();

	for( const auto &it: entries )
	{
		filler(buf, it.first, nullptr, 0);
	}

	return 0;
}

/**
 * \brief
 * 	Create an open-context for a given path
 * \details
 *	This call is used to check whether the given flags are okay for
 *	opening the file
 *	
 *	We can set fi->fh (file handle) here and it will be available in any
 *	other operations coming up.
 **/
int xwmfs_open(const char *path, struct fuse_file_info *fi)
{
	xwmfs::FileSysReadGuard read_guard( *xwmfs::filesystem );

	xwmfs::Entry *entry = xwmfs::filesystem->findEntry( path );

	// if entry is a directory then we allow the open call but during any
	// read/writes we return EISDIR

	// if entry not there at all
	if( ! entry )
	{
		xwmfs::StdLogger::getInstance().debug()
			<< __FUNCTION__ << " didn't find " << path << "\n";
		return -ENOENT;
	}
	// don't allow any write access if entity is not writeable
	else if((fi->flags & 3) != O_RDONLY && !entry->isWritable() )
		return -EACCES;

	// store the pointer to the entry in the file handle
	fi->fh = (intptr_t)entry;

	// increase the reference count to avoid deletion while the file is
	// opened
	//
	// NOTE: this race conditions only shows if fuse is mounted with the
	// direct_io option, otherwise heavy caching is employed that avoids
	// the SEGFAULT when somebody tries to read from an Entry that's
	// already been deleted
	entry->ref();

	return 0;
}

/**
 * \brief
 * 	This is the counter part to xmwfs_open(), called as soon as a user of
 * 	a given file object closes it's file descriptor
 **/
int xwmfs_release(const char *path, struct fuse_file_info *fi)
{
	(void)path;

	xwmfs::FileSysReadGuard read_guard( *xwmfs::filesystem );

	// get our entry pointer back from the file handle field
	xwmfs::Entry *entry = reinterpret_cast<xwmfs::Entry*>(fi->fh);

	// deleting this entry should not require a write guard, because
	// we're the last user of the file nobody else should know about it
	// ...
	if( entry->unref() )
	{
		delete entry;
	}

	// return value is ignored by FUSE
	return 0;
}

int xwmfs_read(
	const char *path, char *buf, size_t size,
	off_t offset, struct fuse_file_info *fi)
{
	(void)path;

	xwmfs::FileSysReadGuard read_guard( *xwmfs::filesystem );

	// get our entry pointer back from the file handle field
	xwmfs::Entry *entry = reinterpret_cast<xwmfs::Entry*>(fi->fh);

	assert(entry);

	try
	{
		auto res = entry->isOperationAllowed();

		return res ? res : entry->read(buf, size, offset);
	}
	catch( const xwmfs::Exception &ex )
	{
		xwmfs::StdLogger::getInstance().error()
			<< "Failed to read from " << path << ": " << ex.what() << "\n";
		return -EFAULT;
	}
}

int xwmfs_write(
	const char *path, const char *buf, size_t size,
	off_t offset, struct fuse_file_info *fi)
{
	(void)path;

	xwmfs::FileSysReadGuard read_guard( *xwmfs::filesystem );

	// get our entry pointer back from the file handle field
	xwmfs::Entry *entry = reinterpret_cast<xwmfs::Entry*>(fi->fh);

	try
	{
		auto res = entry->isOperationAllowed();

		return res ? res : entry->write(buf, size, offset);
	}
	catch( const xwmfs::Exception &ex )
	{
		xwmfs::StdLogger::getInstance().error()
			<< "Failed to write to " << path << ": " << ex.what() << "\n";
		return -EFAULT;
	}
}

int xwmfs_truncate(const char *path, off_t size)
{
	// do nothing
	//
	// note: doing nothing on a "proc like fs" is okay I guess
	//
	// If you try to append or truncate a writable file on proc then
	// nothing happens. We simply implement "overwrite" all the time.
	//
	// This null implementation is needed for shell operations to succeed
	// that try to truncate a file upon writing.
	(void)path;
	(void)size;
	return 0;
}

/**
 * \brief
 * 	Request to create a file on the file system
 * \details
 * 	Right now we have no feature that allows creating a file. Thus we
 * 	return EROFS, read-only file system error.
 * \note
 * 	On kernels < 2.6.15 mknod() and open() will be called instead of
 * 	create.
 **/
int xwmfs_create(
	const char *path,
	mode_t mode,
	struct fuse_file_info *ffi)
{
	(void)path;
	(void)mode;
	(void)ffi;
	return -EROFS;
}

/**
 *  \brief
 *  	File system initialization
 *  \details
 *  	This function is called from FUSE to setup the file system.
 *
 *  	We initialize the XWMFS. It will gather all window manager related
 *  	information and build the file system from it. We set our global file
 *  	system pointer to that file system for further FUSE processing.
 *  \return
 *  	The returned value is passed to all other operations in the
 *  	fuse_context structure and also to xwmfs_destroy(void*)
 **/
void* xwmfs_init(struct fuse_conn_info* conn)
{
	(void)conn;

	try
	{
		xwmfs::Xwmfs &xwmfs = xwmfs::Xwmfs::getInstance();

		const int xwmfs_res = xwmfs.init();

		if( xwmfs_res != EXIT_SUCCESS )
		{
			::exit(xwmfs_res);
		}

		xwmfs::filesystem = &xwmfs.getFS();
	}
	catch( xwmfs::Exception &e )
	{
		xwmfs::StdLogger::getInstance().error()
			<< "Error setting up XWMFS. Exception caught: "
			<< e.what() << "\n";
	}
	catch(...)
	{
		xwmfs::StdLogger::getInstance().error()
			<< "Error setting up XWMFS."
			" Unknown exception caught\n";
	}

	assert( xwmfs::filesystem );

	// we return our file system instance such that we can access it
	// anywhere
	return xwmfs::filesystem;
}

/**
 * \brief
 *	File System cleanup
 * \details
 * 	This function is called by FUSE to cleanup the file system
 **/
void xwmfs_destroy(void* data)
{
	(void)data;

	xwmfs::Xwmfs::getInstance().exit();

	xwmfs::filesystem = nullptr;
}

