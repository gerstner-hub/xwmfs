AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = xwmfs
xwmfs_SOURCES = \
		common/Helper.cxx common/ILogger.cxx common/Exception.cxx common/Mutex.cxx \
		common/Thread.cxx common/strerror.cxx \
		fuse/xwmfs_fuse_ops.c fuse/xwmfs_fuse_ops_impl.cxx fuse/Entry.cxx \
		fuse/FileEntry.cxx fuse/DirEntry.cxx fuse/RootEntry.cxx \
		fuse/SymlinkEntry.cxx fuse/EventFile.cxx fuse/AbortHandler.cxx \
		main/Xwmfs.cxx main/main.cxx main/StdLogger.cxx main/terminate.cxx main/WindowDirEntry.cxx \
		main/WindowFileEntry.cxx main/WinManagerFileEntry.cxx main/WinManagerDirEntry.cxx \
		main/WindowsRootDir.cxx main/UpdateableDir.cxx main/SelectionDirEntry.cxx \
		main/SelectionOwnerFile.cxx main/SelectionAccessFile.cxx \
		main/DesktopsRootDir.cxx main/DesktopDirEntry.cxx \
		x11/XAtom.cxx x11/PropertyTraits.cxx x11/XWindow.cxx x11/XWindowAttrs.cxx \
		x11/XDisplay.cxx x11/RootWin.cxx
xwmfs_SOURCES += \
		common/Condition.hxx common/Exception.hxx common/Helper.hxx common/ILogger.hxx \
		common/IThreadEntry.hxx common/Mutex.hxx common/RWLock.hxx common/SystemException.hxx \
		common/Thread.hxx \
		fuse/xwmfs_fuse_ops.h fuse/AbortHandler.hxx fuse/DirEntry.hxx fuse/Entry.hxx \
		fuse/EventFile.hxx fuse/FileEntry.hxx fuse/OpenContext.hxx fuse/RootEntry.hxx \
		fuse/SymlinkEntry.hxx fuse/xwmfs_fuse.hxx \
		main/Options.hxx \
		main/SelectionAccessFile.hxx main/SelectionDirEntry.hxx main/SelectionOwnerFile.hxx \
		main/StdLogger.hxx main/UpdateableDir.hxx main/WinManagerDirEntry.hxx \
		main/WinManagerFileEntry.hxx main/WindowDirEntry.hxx main/WindowFileEntry.hxx \
		main/WindowsRootDir.hxx main/Xwmfs.hxx \
		main/DesktopsRootDir.hxx main/DesktopDirEntry.hxx \
		x11/RootWin.hxx x11/X11Exception.hxx x11/XAtom.hxx x11/XDisplay.hxx x11/XWindow.hxx \
		x11/XWindowAttrs.hxx x11/Property.hxx x11/PropertyTraits.hxx x11/utf8_string.hxx
# we need x11 and fuse
xwmfs_DEPENDENCIES = x11 fuse
# makes it possible to include headers from fuse or x11, to select a recent fuse API version
AM_CFLAGS = -DFUSE_USE_VERSION=26 @fuse_CFLAGS@ @x11_CFLAGS@ -I${top_srcdir}/src
if DEV
AM_CFLAGS += -Wall -Werror -Wextra -Wnull-dereference -Wdouble-promotion -Wshadow -Wformat=2

# add some more advanced warnings for gcc
if IS_GCC
AM_CFLAGS += -Wduplicated-cond -Wduplicated-branches -Wlogical-op
else
AM_CFLAGS += -Wno-unused-parameter
endif # IS_GCC
endif # DEV

AM_CXXFLAGS =${AM_CFLAGS} -std=c++11
# use this instead of AM_LDFLAGS to have the libraries appear AFTER the object
# files. Otherwise we get trouble on distros where as-needed linking is
# enabled
xwmfs_LDADD = @fuse_LIBS@ @x11_LIBS@

