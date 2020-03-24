AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = xwmfs
xwmfs_SOURCES = fuse/xwmfs_fuse_ops.c fuse/xwmfs_fuse_ops_impl.cxx fuse/Entry.cxx fuse/FileEntry.cxx fuse/DirEntry.cxx fuse/RootEntry.cxx fuse/EventFile.cxx fuse/AbortHandler.cxx x11/XAtom.cxx x11/property.cxx x11/XWindow.cxx x11/XWindowAttrs.cxx x11/XDisplay.cxx x11/RootWin.cxx main/Xwmfs.cxx main/main.cxx main/StdLogger.cxx main/terminate.cxx main/WindowDirEntry.cxx main/WindowFileEntry.cxx main/WinManagerFileEntry.cxx main/WinManagerDirEntry.cxx main/WindowsRootDir.cxx main/UpdateableDir.cxx main/SelectionDirEntry.cxx main/SelectionOwnerFile.cxx main/SelectionAccessFile.cxx common/Helper.cxx common/ILogger.cxx common/Exception.cxx common/Mutex.cxx common/Thread.cxx 
xwmfs_SOURCES += fuse/xwmfs_fuse_ops.h common/Condition.hxx common/Exception.hxx common/Helper.hxx common/ILogger.hxx common/IThreadEntry.hxx common/Mutex.hxx common/RWLock.hxx common/SystemException.hxx common/Thread.hxx fuse/AbortHandler.hxx fuse/DirEntry.hxx fuse/Entry.hxx fuse/EventFile.hxx fuse/FileEntry.hxx fuse/OpenContext.hxx fuse/RootEntry.hxx fuse/xwmfs_fuse.hxx main/Options.hxx main/SelectionAccessFile.hxx main/SelectionDirEntry.hxx main/SelectionOwnerFile.hxx main/StdLogger.hxx main/UpdateableDir.hxx main/WinManagerDirEntry.hxx main/WinManagerFileEntry.hxx main/WindowDirEntry.hxx main/WindowFileEntry.hxx main/WindowsRootDir.hxx main/Xwmfs.hxx x11/RootWin.hxx x11/X11Exception.hxx x11/XAtom.hxx x11/XDisplay.hxx x11/XWindow.hxx x11/XWindowAttrs.hxx x11/property.hxx
# we need x11 and fuse
xwmfs_DEPENDENCIES = x11 fuse
# makes it possible to include headers from fuse or x11, to select a recent fuse API version
AM_CFLAGS = -Wall -Werror -Wextra -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wnull-dereference -Wdouble-promotion -Wshadow -Wformat=2 -I${top_srcdir}/src/ -DFUSE_USE_VERSION=26 @fuse_CFLAGS@ @x11_CFLAGS@
AM_CXXFLAGS =${AM_CFLAGS} -std=c++11
# use this instead of AM_LDFLAGS to have the libraries appear AFTER the object
# files. Otherwise we get trouble on distros where as-needed linking is
# enabled
xwmfs_LDADD = @fuse_LIBS@ @x11_LIBS@

