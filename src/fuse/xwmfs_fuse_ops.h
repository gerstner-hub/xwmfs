#ifndef XWMFS_FUSE_H
#define XWMFS_FUSE_H

#include <fuse.h>
#include <fcntl.h>

/*
 * This is a C/C++ header that declares the fuse operations implemented for
 * xwmfs
 *
 * It needs to be includeable by C, because the definition of struct
 * fuse_operations is done in C, as in C99 we can have designated initializers
 * (as opposed to C++) which makes the setup of that structure way easier for
 * us then if done in C++.
 */

#ifdef __cplusplus
extern "C"
{
#endif

extern int xwmfs_getattr(
	const char *path,
	struct stat *stbuf
);

extern int xwmfs_readdir(
	const char *path,
	void *buf,
	fuse_fill_dir_t filler,
	off_t offset,
	struct fuse_file_info *fi
);

extern int xwmfs_open(
	const char *path,
	struct fuse_file_info *fi
);

extern int xwmfs_release(
	const char *path,
	struct fuse_file_info *fi
);

extern int xwmfs_read(
	const char *path,
	char *buf,
	size_t size,
	off_t offset,
	struct fuse_file_info *fi
);

extern int xwmfs_write(
	const char *path,
	const char *data,
	size_t size,
	off_t offset,
	struct fuse_file_info *fi
);

extern int xwmfs_truncate(
	const char *path,
	off_t size
);

extern void* xwmfs_init(
	struct fuse_conn_info*
);

extern void xwmfs_destroy(
	void*
);

extern int xwmfs_create(
	const char *path,
	mode_t mode,
	struct fuse_file_info *ffi
);

extern struct fuse_operations xwmfs_oper;

#ifdef __cplusplus
}
#endif

#endif // inc. guard
