AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = xwmfs
xwmfs_SOURCES = fuse/xwmfs_fuse_ops.c fuse/xwmfs_fuse_ops_impl.cxx fuse/xwmfs_fuse.cxx x11/XAtom.cxx x11/property.cxx x11/XWindow.cxx x11/RootWin.cxx main/Xwmfs.cxx main/main.cxx main/StdLogger.cxx main/terminate.cxx common/Helper.cxx common/ILogger.cxx common/Exception.cxx common/Mutex.cxx common/Thread.cxx
# we need x11 and fuse
xwmfs_DEPENDENCIES = x11 fuse
# makes it possible to include headers from fuse or x11, to select a recent fuse API version
AM_CFLAGS = -Wall -Werror -Wextra -I${top_srcdir}/src/ -DFUSE_USE_VERSION=26 @fuse_CFLAGS@ @x11_CFLAGS@
AM_CXXFLAGS =${AM_CFLAGS} -std=c++11
# use this instead of AM_LDFLAGS to have the libraries appear AFTER the object
# files. Otherwise we get trouble on distros where as-needed linking is
# enabled
xwmfs_LDADD = @fuse_LIBS@ @x11_LIBS@

