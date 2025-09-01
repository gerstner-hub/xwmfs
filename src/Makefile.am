AUTOMAKE_OPTIONS = subdir-objects

noinst_LTLIBRARIES = libcosmos.la libxpp.la
libcosmos_la_SOURCES = libcosmos/include/string.hxx libcosmos/include/cosmos.hxx libcosmos/include/types.hxx \
	libcosmos/include/net/SocketError.hxx libcosmos/include/net/InterfaceEnumerator.hxx libcosmos/include/net/InterfaceIterator.hxx \
	libcosmos/include/net/Socket.hxx libcosmos/include/net/UnixConnection.hxx libcosmos/include/net/InterfaceAddress.hxx \
	libcosmos/include/net/types.hxx libcosmos/include/net/SocketAddress.hxx libcosmos/include/net/AddressInfoList.hxx \
	libcosmos/include/net/SockOptBase.hxx libcosmos/include/net/UDPOptions.hxx libcosmos/include/net/unix_aux.hxx \
	libcosmos/include/net/UnixClientSocket.hxx libcosmos/include/net/LinkLayerAddress.hxx libcosmos/include/net/IPOptions.hxx \
	libcosmos/include/net/TCPOptions.hxx libcosmos/include/net/byte_order.hxx libcosmos/include/net/UnixOptions.hxx \
	libcosmos/include/net/InterfaceAddressIterator.hxx libcosmos/include/net/InterfaceInfo.hxx libcosmos/include/net/ListenSocket.hxx \
	libcosmos/include/net/InterfaceAddressList.hxx libcosmos/include/net/AddressInfoIterator.hxx libcosmos/include/net/UnixListenSocket.hxx \
	libcosmos/include/net/UDPSocket.hxx libcosmos/include/net/ip_aux.hxx libcosmos/include/net/traits.hxx libcosmos/include/net/UnixDatagramSocket.hxx \
	libcosmos/include/net/message_header.hxx libcosmos/include/net/tcp.hxx libcosmos/include/net/TCPConnection.hxx libcosmos/include/net/IPSocket.hxx \
	libcosmos/include/net/TCPClientSocket.hxx libcosmos/include/net/AddressInfo.hxx libcosmos/include/net/AddressHints.hxx libcosmos/include/net/UnixAddress.hxx \
	libcosmos/include/net/network.hxx libcosmos/include/net/IPAddress.hxx libcosmos/include/net/TCPListenSocket.hxx libcosmos/include/net/SocketOptions.hxx \
	libcosmos/include/BitMask.hxx libcosmos/include/thread/pthread.hxx libcosmos/include/thread/thread.hxx libcosmos/include/thread/Condition.hxx \
	libcosmos/include/thread/RWLock.hxx libcosmos/include/thread/PosixThread.hxx libcosmos/include/thread/Mutex.hxx libcosmos/include/GroupInfo.hxx \
	libcosmos/include/time/types.hxx libcosmos/include/time/Clock.hxx libcosmos/include/time/TimerFD.hxx libcosmos/include/time/StopWatch.hxx \
	libcosmos/include/time/time.hxx libcosmos/include/PasswdInfo.hxx libcosmos/include/compiler.hxx libcosmos/include/utils.hxx \
	libcosmos/include/locale.hxx libcosmos/include/InfoBase.hxx libcosmos/include/limits.hxx libcosmos/include/dso_export.h \
	libcosmos/include/proc/SigAction.hxx libcosmos/include/proc/types.hxx libcosmos/include/proc/ChildCloner.hxx libcosmos/include/proc/process.hxx \
	libcosmos/include/proc/Scheduler.hxx libcosmos/include/proc/SigInfo.hxx libcosmos/include/proc/signal.hxx libcosmos/include/proc/mman.hxx \
	libcosmos/include/proc/ptrace.hxx libcosmos/include/proc/SigSet.hxx libcosmos/include/proc/Tracee.hxx libcosmos/include/proc/pidfd.h \
	libcosmos/include/proc/clone.hxx libcosmos/include/proc/ProcessFile.hxx libcosmos/include/proc/SignalFD.hxx libcosmos/include/proc/Mapping.hxx \
	libcosmos/include/proc/SubProc.hxx libcosmos/include/proc/PidFD.hxx libcosmos/include/fs/DirFD.hxx libcosmos/include/fs/FileBase.hxx \
	libcosmos/include/fs/types.hxx libcosmos/include/fs/TempDir.hxx libcosmos/include/fs/DirIterator.hxx libcosmos/include/fs/FileDescriptor.hxx \
	libcosmos/include/fs/TempFile.hxx libcosmos/include/fs/filesystem.hxx libcosmos/include/fs/FDFile.hxx libcosmos/include/fs/DirStream.hxx \
	libcosmos/include/fs/path.hxx libcosmos/include/fs/DirEntry.hxx libcosmos/include/fs/FileStatus.hxx libcosmos/include/fs/FileLock.hxx \
	libcosmos/include/fs/File.hxx libcosmos/include/fs/Directory.hxx libcosmos/include/memory.hxx libcosmos/include/SysString.hxx \
	libcosmos/include/main.hxx libcosmos/include/io/iovector.hxx libcosmos/include/io/StdLogger.hxx libcosmos/include/io/ILogger.hxx \
	libcosmos/include/io/types.hxx libcosmos/include/io/SecretFile.hxx libcosmos/include/io/MemFile.hxx libcosmos/include/io/StreamIO.hxx \
	libcosmos/include/io/Pipe.hxx libcosmos/include/io/SecretMemFile.hxx libcosmos/include/io/colors.hxx libcosmos/include/io/Terminal.hxx \
	libcosmos/include/io/Poller.hxx libcosmos/include/io/EventFile.hxx libcosmos/include/io/StreamAdaptor.hxx libcosmos/include/formatting.hxx \
	libcosmos/include/fwd.hxx libcosmos/include/error/UsageError.hxx libcosmos/include/error/errno.hxx libcosmos/include/error/macros.hxx \
	libcosmos/include/error/FileError.hxx libcosmos/include/error/InternalError.hxx libcosmos/include/error/CosmosError.hxx libcosmos/include/error/RangeError.hxx \
	libcosmos/include/error/ResolveError.hxx libcosmos/include/error/RuntimeError.hxx libcosmos/include/error/WouldBlock.hxx libcosmos/include/error/ApiError.hxx \
	libcosmos/src/string.cxx libcosmos/src/PasswdInfo.cxx libcosmos/src/net/ip_aux.cxx libcosmos/src/net/AddressInfoList.cxx \
	libcosmos/src/net/LinkLayerAddress.cxx libcosmos/src/net/Socket.cxx libcosmos/src/net/UnixAddress.cxx libcosmos/src/net/SockOptBase.cxx \
	libcosmos/src/net/AddressInfo.cxx libcosmos/src/net/SocketOptions.cxx libcosmos/src/net/UnixClientSocket.cxx libcosmos/src/net/InterfaceEnumerator.cxx \
	libcosmos/src/net/IPOptions.cxx libcosmos/src/net/TCPOoptions.cxx libcosmos/src/net/InterfaceAddressList.cxx libcosmos/src/net/network.cxx \
	libcosmos/src/net/UnixOptions.cxx libcosmos/src/net/unix_aux.cxx libcosmos/src/net/UnixListenSocket.cxx libcosmos/src/net/IPAddress.cxx \
	libcosmos/src/net/message_header.cxx libcosmos/src/net/SocketAddress.cxx libcosmos/src/cosmos.cxx libcosmos/src/GroupInfo.cxx \
	libcosmos/src/thread/thread.cxx libcosmos/src/thread/PosixThread.cxx libcosmos/src/thread/Mutex.cxx libcosmos/src/thread/Condition.cxx \
	libcosmos/src/thread/pthread.cxx libcosmos/src/SysString.cxx libcosmos/src/time/TimerFD.cxx libcosmos/src/time/Clock.cxx \
	libcosmos/src/locale.cxx libcosmos/src/proc/SubProc.cxx libcosmos/src/proc/Scheduler.cxx libcosmos/src/proc/signal.cxx \
	libcosmos/src/proc/mman.cxx libcosmos/src/proc/ProcessFile.cxx libcosmos/src/proc/process.cxx libcosmos/src/proc/SigAction.cxx \
	libcosmos/src/proc/SigInfo.cxx libcosmos/src/proc/ptrace.cxx libcosmos/src/proc/Tracee.cxx libcosmos/src/proc/ChildCloner.cxx \
	libcosmos/src/proc/types.cxx libcosmos/src/proc/SignalFD.cxx libcosmos/src/proc/clone.cxx libcosmos/src/fs/filesystem.cxx \
	libcosmos/src/fs/TempFile.cxx libcosmos/src/fs/path.cxx libcosmos/src/fs/File.cxx libcosmos/src/fs/Directory.cxx \
	libcosmos/src/fs/FileDescriptor.cxx libcosmos/src/fs/FileStatus.cxx libcosmos/src/fs/DirStream.cxx libcosmos/src/fs/FDFile.cxx \
	libcosmos/src/fs/FileBase.cxx libcosmos/src/fs/TempDir.cxx libcosmos/src/fs/FileLock.cxx libcosmos/src/io/iovector.cxx \
	libcosmos/src/io/colors.cxx libcosmos/src/io/StdLogger.cxx libcosmos/src/io/MemFile.cxx libcosmos/src/io/Terminal.cxx \
	libcosmos/src/io/StreamIO.cxx libcosmos/src/io/SecretFile.cxx libcosmos/src/io/Poller.cxx libcosmos/src/io/Pipe.cxx \
	libcosmos/src/io/EventFile.cxx libcosmos/src/io/ILogger.cxx libcosmos/src/error/errno.cxx libcosmos/src/error/ApiError.cxx \
	libcosmos/src/error/CosmosError.cxx libcosmos/src/error/FileError.cxx libcosmos/src/error/ResolveError.cxx \
	libcosmos/src/error/strerror.cxx libcosmos/src/formatting.cxx

libxpp_la_SOURCES = libxpp/include/helpers.hxx libxpp/include/Xpp.hxx libxpp/include/Event.hxx \
	libxpp/include/types.hxx libxpp/include/Pixmap.hxx libxpp/include/SetWindowAttributes.hxx \
	libxpp/include/Property.hxx libxpp/include/XColor.hxx libxpp/include/RootWin.hxx \
	libxpp/include/XCursor.hxx libxpp/include/CachedAtom.hxx libxpp/include/PropertyTraits.hxx \
	libxpp/include/X11Exception.hxx libxpp/include/event/ClientMessageEvent.hxx libxpp/include/event/PointerMovedEvent.hxx \
	libxpp/include/event/InputEvent.hxx libxpp/include/event/ButtonEvent.hxx libxpp/include/event/KeyEvent.hxx \
	libxpp/include/event/VisibilityEvent.hxx libxpp/include/event/ConfigureEvent.hxx libxpp/include/event/FocusChangeEvent.hxx \
	libxpp/include/event/PropertyEvent.hxx libxpp/include/event/SelectionRequestEvent.hxx libxpp/include/event/SelectionEvent.hxx \
	libxpp/include/WindowManagerHints.hxx libxpp/include/utils.hxx libxpp/include/SizeHints.hxx \
	libxpp/include/utf8_string.hxx libxpp/include/dso_export.h libxpp/include/AtomMapper.hxx \
	libxpp/include/XWindow.hxx libxpp/include/GraphicsContext.hxx libxpp/include/XDisplay.hxx \
	libxpp/include/formatting.hxx libxpp/include/ClassHints.hxx libxpp/include/XWindowAttrs.hxx \
	libxpp/include/keyboard.hxx libxpp/include/fwd.hxx libxpp/include/atoms.hxx \
	libxpp/src/GraphicsContext.cxx libxpp/src/XWindow.cxx libxpp/src/Pixmap.cxx \
	libxpp/src/XWindowAttrs.cxx libxpp/src/Event.cxx libxpp/src/Xpp.cxx \
	libxpp/src/PropertyTraits.cxx libxpp/src/helpers.cxx libxpp/src/XCursor.cxx \
	libxpp/src/keyboard.cxx libxpp/src/CachedAtom.cxx libxpp/src/RootWin.cxx \
	libxpp/src/XDisplay.cxx libxpp/src/AtomMapper.cxx libxpp/src/formatting.cxx

bin_PROGRAMS = xwmfs
xwmfs_SOURCES = \
		fuse/xwmfs_fuse_ops.c fuse/xwmfs_fuse_ops_impl.cxx fuse/Entry.cxx \
		fuse/FileEntry.cxx fuse/DirEntry.cxx fuse/RootEntry.cxx \
		fuse/SymlinkEntry.cxx fuse/EventFile.cxx fuse/AbortHandler.cxx \
		main/Xwmfs.cxx main/main.cxx main/logger.cxx main/terminate.cxx main/WindowDirEntry.cxx \
		main/WindowFileEntry.cxx main/WinManagerFileEntry.cxx main/WinManagerDirEntry.cxx \
		main/WindowsRootDir.cxx main/UpdatableDir.cxx main/SelectionDirEntry.cxx \
		main/SelectionOwnerFile.cxx main/SelectionAccessFile.cxx \
		main/DesktopsRootDir.cxx main/DesktopDirEntry.cxx \
		x11/WinManagerWindow.cxx
xwmfs_SOURCES += \
		fuse/xwmfs_fuse_ops.h fuse/AbortHandler.hxx fuse/DirEntry.hxx fuse/Entry.hxx \
		fuse/EventFile.hxx fuse/FileEntry.hxx fuse/OpenContext.hxx fuse/RootEntry.hxx \
		fuse/SymlinkEntry.hxx fuse/xwmfs_fuse.hxx \
		main/Options.hxx \
		main/SelectionAccessFile.hxx main/SelectionDirEntry.hxx main/SelectionOwnerFile.hxx \
		main/logger.hxx main/UpdatableDir.hxx main/WinManagerDirEntry.hxx \
		main/WinManagerFileEntry.hxx main/WindowDirEntry.hxx main/WindowFileEntry.hxx \
		main/WindowsRootDir.hxx main/Xwmfs.hxx \
		main/DesktopsRootDir.hxx main/DesktopDirEntry.hxx \
		x11/WinManagerWindow.hxx
# we need x11 and fuse
xwmfs_DEPENDENCIES = x11 fuse libcosmos.la libxpp.la

if DEV
AM_CFLAGS = -Wall -Werror -Wextra -Wnull-dereference -Wdouble-promotion -Wshadow -Wformat=2

# add some more advanced warnings for gcc
if IS_GCC
AM_CFLAGS += -Wduplicated-cond -Wduplicated-branches -Wlogical-op
else
AM_CFLAGS += -Wno-unused-parameter
endif # IS_GCC
else
AM_CFLAGS =
endif # DEV

AM_CXXFLAGS = ${AM_CFLAGS} -std=c++20
libcosmos_la_CXXFLAGS = ${AM_CXXFLAGS} -I${top_srcdir}/src/libcosmos/src -I${top_srcdir}/src/libcosmos/include
libxpp_la_CXXFLAGS = ${libcosmos_la_CXXFLAGS} -I${top_srcdir}/src/libxpp/include -I${top_srcdir}/src/libxpp/src
# makes it possible to include headers from fuse or x11, to select a recent fuse API version
xwmfs_CFLAGS = ${AM_CFLAGS} -DFUSE_USE_VERSION=35 @fuse3_CFLAGS@ @x11_CFLAGS@ -I${top_srcdir}/src
xwmfs_CXXFLAGS = ${xwmfs_CFLAGS} -I${top_srcdir}/src/libxpp/include -I${top_srcdir}/src/libcosmos/include -std=c++20
# use this instead of AM_LDFLAGS to have the libraries appear AFTER the object
# files. Otherwise we get trouble on distros where as-needed linking is
# enabled
xwmfs_LDADD = @fuse3_LIBS@ @x11_LIBS@ libcosmos.la libxpp.la
