/*
 * This is the implementation of the fuse specific functions that make xwmfs
 * actually work as a FS.
 *
 * The file system operations are called directly by FUSE as soon as some
 * access to the file system occurs.
 */

// C/C++
#include <assert.h>
#include <errno.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

// xwmfs
#include "common/Exception.hxx"
#include "fuse/Entry.hxx"
#include "fuse/FileEntry.hxx"
#include "fuse/OpenContext.hxx"
#include "fuse/xwmfs_fuse.hxx"
#include "fuse/xwmfs_fuse_ops.h"
#include "main/Xwmfs.hxx"

namespace xwmfs {

xwmfs::RootEntry *filesystem = nullptr;

static OpenContext* context_from_fi(struct fuse_file_info *fi) {
	// get our entry pointer back from the file handle field
	return reinterpret_cast<xwmfs::OpenContext*>(fi->fh);
}

static Entry* entry_from_fi(struct fuse_file_info *fi) {
	xwmfs::OpenContext *context = context_from_fi(fi);
	return context->getEntry();
}

} // end ns

/**
 * \brief
 * 	Get stat information about a file system entry
 * \details
 * 	stat may happen with or without file open context in `fi`.
 **/
int xwmfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
	memset(stbuf, 0, sizeof(struct stat));
	xwmfs::FileSysReadGuard read_guard{*xwmfs::filesystem};

	const xwmfs::Entry *entry = fi ? xwmfs::entry_from_fi(fi) : xwmfs::filesystem->findEntry(path);
	if (!entry) {
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
int xwmfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi,
		enum fuse_readdir_flags flags) {
	(void) offset;
	(void) fi;

	xwmfs::FileSysReadGuard read_guard{*xwmfs::filesystem};

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
	xwmfs::Entry *entry = xwmfs::filesystem->findEntry(path);

	xwmfs::DirEntry *dir_entry = xwmfs::Entry::tryCastDirEntry(entry);

	if (!entry) {
		xwmfs::StdLogger::getInstance().debug()
			<< __FUNCTION__ << ": no such entity: "
			<< path << "\n";

		return -ENOENT;
	} else if (!dir_entry) {
		xwmfs::StdLogger::getInstance().debug()
			<< __FUNCTION__ << ": not a dir: "
			<< path << "\n";
		return -ENOTDIR;
	}

	// okay we found a valid directory to list the contents of
	const auto &entries = dir_entry->getEntries();

	/*
	 * the kernel would like stat information for each entry right away.
	 */
	const bool provide_stat = (flags & FUSE_READDIR_PLUS) != 0;
	const enum fuse_fill_dir_flags fill_flags = provide_stat ? FUSE_FILL_DIR_PLUS : FUSE_FILL_DIR_DEFAULTS;
	struct stat stbuf;

	for (const auto &it: entries) {
		if (provide_stat) {
			it.second->getStat(&stbuf);
		}

		filler(buf, it.first, provide_stat ? &stbuf : nullptr, 0, fill_flags);
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
int xwmfs_open(const char *path, struct fuse_file_info *fi) {
	xwmfs::FileSysReadGuard read_guard{*xwmfs::filesystem};

	xwmfs::Entry *entry = xwmfs::filesystem->findEntry(path);

	// if entry is a directory then we allow the open call but during any
	// read/writes we return EISDIR

	if (!entry) {
		// if entry not there at all
		xwmfs::StdLogger::getInstance().debug()
			<< __FUNCTION__ << " didn't find " << path << "\n";
		return -ENOENT;
	} else if((fi->flags & 3) != O_RDONLY && !entry->isWritable()) {
		// don't allow any write access if entity is not writeable
		return -EACCES;
	}

	auto ctx = entry->createOpenContext();

	if (fi->flags & O_NONBLOCK) {
		ctx->setNonBlocking(true);
	}

	// store a pointer to an OpenContext in the file handle
	fi->fh = (intptr_t)ctx;

	if (entry->enableDirectIO()) {
		fi->direct_io = 1;
	}

	return 0;
}

/**
 * \brief
 * 	This is the counter part to xmwfs_open(), called as soon as a user of
 * 	a given file object closes it's file descriptor
 **/
int xwmfs_release(const char *path, struct fuse_file_info *fi) {
	(void)path;

	xwmfs::FileSysReadGuard read_guard{*xwmfs::filesystem};

	auto *context = xwmfs::context_from_fi(fi);
	auto entry = context->getEntry();

	entry->destroyOpenContext(context);

	// return value is ignored by FUSE
	return 0;
}

int xwmfs_read(const char *path, char *buf, size_t size,
		off_t offset, struct fuse_file_info *fi) {
	(void)path;

	xwmfs::FileSysReadGuard read_guard{*xwmfs::filesystem};

	// get our context pointer back from the file handle field
	auto context = xwmfs::context_from_fi(fi);
	auto entry = context->getEntry();

	try {
		auto res = entry->isOperationAllowed();

		return res ? res : entry->read(context, buf, size, offset);
	} catch (const xwmfs::Exception &ex) {
		xwmfs::StdLogger::getInstance().error()
			<< "Failed to read from " << path << ": " << ex.what() << "\n";
		return -EFAULT;
	}
}

int xwmfs_readlink(const char *path, char *buf, size_t size) {
	auto &fs = *xwmfs::filesystem;
	xwmfs::FileSysReadGuard read_guard{fs};

	auto entry = fs.findEntry(path);

	try {
		auto res = entry->isOperationAllowed();

		return res ? res : entry->readlink(buf, size);
	} catch (const xwmfs::Exception &ex) {
		xwmfs::StdLogger::getInstance().error()
			<< "Failed to readlink from " << path << ": " << ex.what() << "\n";
		return -EFAULT;
	}
}

int xwmfs_write(const char *path, const char *buf, size_t size,
		off_t offset, struct fuse_file_info *fi) {
	(void)path;

	xwmfs::FileSysReadGuard read_guard{*xwmfs::filesystem};

	auto context = xwmfs::context_from_fi(fi);
	auto entry = context->getEntry();

	try {
		auto res = entry->isOperationAllowed();

		return res ? res : entry->write(context, buf, size, offset);
	} catch(const xwmfs::Exception &ex) {
		xwmfs::StdLogger::getInstance().error()
			<< "Failed to write to " << path << ": " << ex.what() << "\n";
		return -EFAULT;
	}
}

int xwmfs_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
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
	(void)fi;
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
int xwmfs_create(const char *path, mode_t mode, struct fuse_file_info *ffi) {
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
void* xwmfs_init(struct fuse_conn_info *conn, struct fuse_config *config) {
	(void)conn;

	// make interruptible the default, this seems to be the only way.
	// Otherwise the abort logic for blocking calls is not enabled
	config->intr = 1;

	try {
		xwmfs::Xwmfs &xwmfs = xwmfs::Xwmfs::getInstance();

		const int xwmfs_res = xwmfs.init();

		if (xwmfs_res != EXIT_SUCCESS) {
			::exit(xwmfs_res);
		}

		xwmfs::filesystem = &xwmfs.getFS();
	} catch (xwmfs::Exception &e) {
		xwmfs::StdLogger::getInstance().error()
			<< "Error setting up XWMFS. Exception caught: "
			<< e.what() << "\n";
	} catch(...) {
		xwmfs::StdLogger::getInstance().error()
			<< "Error setting up XWMFS."
			" Unknown exception caught\n";
	}

	assert(xwmfs::filesystem);

	// we return our file system instance such that we can access it from
	// all contexts.
	return xwmfs::filesystem;
}

/**
 * \brief
 *	File System cleanup
 * \details
 * 	This function is called by FUSE to cleanup the file system
 **/
void xwmfs_destroy(void* data) {
	(void)data;

	xwmfs::Xwmfs::getInstance().exit();

	xwmfs::filesystem = nullptr;
}
