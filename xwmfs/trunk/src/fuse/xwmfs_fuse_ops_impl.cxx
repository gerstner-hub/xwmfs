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

// for strchrnul
//#define _GNU_SOURCE
#include <string.h>

#include <errno.h>
#include <assert.h>

#include "fuse/xwmfs_fuse.hxx"
#include "fuse/xwmfs_fuse_ops.h"
#include "main/Xwmfs.hxx"

// XXX maybe better move the file system instance in here such that FUSE
// doesn't need to include XWMFS headers?

namespace xwmfs
{

// XXX we should have a const variant of that thing?
xwmfs::RootEntry *filesystem = NULL;

}

/**
 * \brief
 * 	Get stat information about a file system entry
 * \details
 * 	stat is done w/o an open context
 **/
int xwmfs_getattr(const char *path, struct stat *stbuf)
{
	xwmfs::FileSysReadGuard read_guard( *xwmfs::filesystem );

	xwmfs::Entry *entry = xwmfs::filesystem->findEntry( path );

	xwmfs::DirEntry *dir_entry = xwmfs::Entry::tryCastDirEntry(entry);

	if( ! entry )
	{
		xwmfs::StdLogger::getInstance().debug()
			<< __FUNCTION__ << ": noent for path "
			<< path << "\n";
		return -ENOENT;
	}

	memset(stbuf, 0, sizeof(struct stat));

	if( dir_entry )
	{
		stbuf->st_mode = S_IFDIR | 0755;
		// a directory is always linked at least twice due to '.'
		stbuf->st_nlink = 2;
	}
	else
	{
		xwmfs::FileEntry *file_entry = xwmfs::Entry::tryCastFileEntry(entry);
		assert(file_entry);
		stbuf->st_mode = S_IFREG | (file_entry->isWriteable() ? 0666 : 0444);
		stbuf->st_nlink = 1;
		// determine size of stream and return it in stat structure
		file_entry->seekg( 0, xwmfs::FileEntry::end );
		stbuf->st_size = file_entry->tellg();
	}

	stbuf->st_atime = stbuf->st_mtime = entry->getModifyTime();
	stbuf->st_ctime = entry->getStatusTime();

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

	// okay we found a valid directory to list the contents from
	const xwmfs::DirEntry::NameEntryMap &entries = dir_entry->getEntries();

	for(
		xwmfs::DirEntry::NameEntryMap::const_iterator e = entries.begin();
		e != entries.end();
		e++ )
	{
		filler(buf, e->first, NULL, 0);
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
	else if((fi->flags & 3) != O_RDONLY && !entry->isWriteable() )
		return -EACCES;

	// now we could store e.g. a pointer to entry in fi
	fi->fh = (intptr_t)entry;

	// XXX problem is: if updates of the xwmfs are added then entry may
	// become invalid. Here we should act like most real filesystems: keep
	// the "entry" existing somewhere until it is closed by clients
	//
	// for this to work we need to add an open counter and a deleted flag
	// or something like it.

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
	
	// reading a directory via read doesn't make much sense
	if( entry->type() == xwmfs::Entry::DIRECTORY )
		return -EISDIR;

	assert( entry->type() == xwmfs::Entry::REG_FILE );

	xwmfs::FileEntry &file = *(xwmfs::Entry::tryCastFileEntry(entry));

	// position to the required offset in the file (to beginning of file, if no offset)
	file.seekg( offset, xwmfs::FileEntry::beg );

	// read data into fuse buffer
	file.read( buf, size );

	const int ret = file.gcount();

	// remove any possible error or EOF states from stream
	file.clear();

	// return number of bytes actually retrieved
	return ret;
}

int xwmfs_write(
	const char *path, const char *data, size_t size,
	off_t offset, struct fuse_file_info *fi)
{
	(void)path;

	// XXX we don't modify the file system structure here, only the file.
	// the file needs it's own lock at that point
	xwmfs::FileSysReadGuard read_guard( *xwmfs::filesystem );

	// get our entry pointer back from the file handle field
	xwmfs::Entry *entry = reinterpret_cast<xwmfs::Entry*>(fi->fh);

	assert(entry);
	
	// writing a directory via write doesn't make much sense
	if( entry->type() == xwmfs::Entry::DIRECTORY )
		return -EISDIR;

	assert( entry->type() == xwmfs::Entry::REG_FILE );

	xwmfs::FileEntry &file = *(xwmfs::Entry::tryCastFileEntry)(entry);

	return file.write(data, size, offset);
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
	
	xwmfs::filesystem = NULL;
}

