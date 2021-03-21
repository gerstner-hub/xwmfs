#include "fuse/xwmfs_fuse_ops.h"

/*
 * This is a C99 only file that simply defines the fuse operations with
 * designated initializers, which we cannot do in C++.
 */

struct fuse_operations xwmfs_oper =
{
	.getattr = xwmfs_getattr,
	.readdir = xwmfs_readdir,
	.open = xwmfs_open,
	.release = xwmfs_release,
	.read = xwmfs_read,
	.readlink = xwmfs_readlink,
	.write = xwmfs_write,
	.truncate = xwmfs_truncate,
	.init = xwmfs_init,
	.destroy = xwmfs_destroy,
	.create = xwmfs_create
};
