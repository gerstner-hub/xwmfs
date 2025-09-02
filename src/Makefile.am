AUTOMAKE_OPTIONS = subdir-objects

noinst_LTLIBRARIES = libcosmos.la libxpp.la
libcosmos_la_SOURCES = libcosmos/include/cosmos/string.hxx libcosmos/include/cosmos/cosmos.hxx libcosmos/include/cosmos/types.hxx \
	libcosmos/include/cosmos/net/SocketError.hxx libcosmos/include/cosmos/net/InterfaceEnumerator.hxx libcosmos/include/cosmos/net/InterfaceIterator.hxx \
	libcosmos/include/cosmos/net/Socket.hxx libcosmos/include/cosmos/net/UnixConnection.hxx libcosmos/include/cosmos/net/InterfaceAddress.hxx \
	libcosmos/include/cosmos/net/types.hxx libcosmos/include/cosmos/net/SocketAddress.hxx libcosmos/include/cosmos/net/AddressInfoList.hxx \
	libcosmos/include/cosmos/net/SockOptBase.hxx libcosmos/include/cosmos/net/UDPOptions.hxx libcosmos/include/cosmos/net/unix_aux.hxx \
	libcosmos/include/cosmos/net/UnixClientSocket.hxx libcosmos/include/cosmos/net/LinkLayerAddress.hxx libcosmos/include/cosmos/net/IPOptions.hxx \
	libcosmos/include/cosmos/net/TCPOptions.hxx libcosmos/include/cosmos/net/byte_order.hxx libcosmos/include/cosmos/net/UnixOptions.hxx \
	libcosmos/include/cosmos/net/InterfaceAddressIterator.hxx libcosmos/include/cosmos/net/InterfaceInfo.hxx libcosmos/include/cosmos/net/ListenSocket.hxx \
	libcosmos/include/cosmos/net/InterfaceAddressList.hxx libcosmos/include/cosmos/net/AddressInfoIterator.hxx libcosmos/include/cosmos/net/UnixListenSocket.hxx \
	libcosmos/include/cosmos/net/UDPSocket.hxx libcosmos/include/cosmos/net/ip_aux.hxx libcosmos/include/cosmos/net/traits.hxx libcosmos/include/cosmos/net/UnixDatagramSocket.hxx \
	libcosmos/include/cosmos/net/message_header.hxx libcosmos/include/cosmos/net/tcp.hxx libcosmos/include/cosmos/net/TCPConnection.hxx libcosmos/include/cosmos/net/IPSocket.hxx \
	libcosmos/include/cosmos/net/TCPClientSocket.hxx libcosmos/include/cosmos/net/AddressInfo.hxx libcosmos/include/cosmos/net/AddressHints.hxx libcosmos/include/cosmos/net/UnixAddress.hxx \
	libcosmos/include/cosmos/net/network.hxx libcosmos/include/cosmos/net/IPAddress.hxx libcosmos/include/cosmos/net/TCPListenSocket.hxx libcosmos/include/cosmos/net/SocketOptions.hxx \
	libcosmos/include/cosmos/BitMask.hxx libcosmos/include/cosmos/thread/pthread.hxx libcosmos/include/cosmos/thread/thread.hxx libcosmos/include/cosmos/thread/Condition.hxx \
	libcosmos/include/cosmos/thread/RWLock.hxx libcosmos/include/cosmos/thread/PosixThread.hxx libcosmos/include/cosmos/thread/Mutex.hxx libcosmos/include/cosmos/GroupInfo.hxx \
	libcosmos/include/cosmos/time/types.hxx libcosmos/include/cosmos/time/Clock.hxx libcosmos/include/cosmos/time/TimerFD.hxx libcosmos/include/cosmos/time/StopWatch.hxx \
	libcosmos/include/cosmos/time/time.hxx libcosmos/include/cosmos/PasswdInfo.hxx libcosmos/include/cosmos/compiler.hxx libcosmos/include/cosmos/utils.hxx \
	libcosmos/include/cosmos/locale.hxx libcosmos/include/cosmos/InfoBase.hxx libcosmos/include/cosmos/limits.hxx libcosmos/include/cosmos/dso_export.h \
	libcosmos/include/cosmos/proc/SigAction.hxx libcosmos/include/cosmos/proc/types.hxx libcosmos/include/cosmos/proc/ChildCloner.hxx libcosmos/include/cosmos/proc/process.hxx \
	libcosmos/include/cosmos/proc/Scheduler.hxx libcosmos/include/cosmos/proc/SigInfo.hxx libcosmos/include/cosmos/proc/signal.hxx libcosmos/include/cosmos/proc/mman.hxx \
	libcosmos/include/cosmos/proc/ptrace.hxx libcosmos/include/cosmos/proc/SigSet.hxx libcosmos/include/cosmos/proc/Tracee.hxx libcosmos/include/cosmos/proc/pidfd.h \
	libcosmos/include/cosmos/proc/clone.hxx libcosmos/include/cosmos/proc/ProcessFile.hxx libcosmos/include/cosmos/proc/SignalFD.hxx libcosmos/include/cosmos/proc/Mapping.hxx \
	libcosmos/include/cosmos/proc/SubProc.hxx libcosmos/include/cosmos/proc/PidFD.hxx libcosmos/include/cosmos/fs/DirFD.hxx libcosmos/include/cosmos/fs/FileBase.hxx \
	libcosmos/include/cosmos/fs/types.hxx libcosmos/include/cosmos/fs/TempDir.hxx libcosmos/include/cosmos/fs/DirIterator.hxx libcosmos/include/cosmos/fs/FileDescriptor.hxx \
	libcosmos/include/cosmos/fs/TempFile.hxx libcosmos/include/cosmos/fs/filesystem.hxx libcosmos/include/cosmos/fs/FDFile.hxx libcosmos/include/cosmos/fs/DirStream.hxx \
	libcosmos/include/cosmos/fs/path.hxx libcosmos/include/cosmos/fs/DirEntry.hxx libcosmos/include/cosmos/fs/FileStatus.hxx libcosmos/include/cosmos/fs/FileLock.hxx \
	libcosmos/include/cosmos/fs/File.hxx libcosmos/include/cosmos/fs/Directory.hxx libcosmos/include/cosmos/memory.hxx libcosmos/include/cosmos/SysString.hxx \
	libcosmos/include/cosmos/main.hxx libcosmos/include/cosmos/io/iovector.hxx libcosmos/include/cosmos/io/StdLogger.hxx libcosmos/include/cosmos/io/ILogger.hxx \
	libcosmos/include/cosmos/io/types.hxx libcosmos/include/cosmos/io/SecretFile.hxx libcosmos/include/cosmos/io/MemFile.hxx libcosmos/include/cosmos/io/StreamIO.hxx \
	libcosmos/include/cosmos/io/Pipe.hxx libcosmos/include/cosmos/io/SecretMemFile.hxx libcosmos/include/cosmos/io/colors.hxx libcosmos/include/cosmos/io/Terminal.hxx \
	libcosmos/include/cosmos/io/Poller.hxx libcosmos/include/cosmos/io/EventFile.hxx libcosmos/include/cosmos/io/StreamAdaptor.hxx libcosmos/include/cosmos/formatting.hxx \
	libcosmos/include/cosmos/fwd.hxx libcosmos/include/cosmos/error/UsageError.hxx libcosmos/include/cosmos/error/errno.hxx libcosmos/include/cosmos/error/macros.hxx \
	libcosmos/include/cosmos/error/FileError.hxx libcosmos/include/cosmos/error/InternalError.hxx libcosmos/include/cosmos/error/CosmosError.hxx libcosmos/include/cosmos/error/RangeError.hxx \
	libcosmos/include/cosmos/error/ResolveError.hxx libcosmos/include/cosmos/error/RuntimeError.hxx libcosmos/include/cosmos/error/WouldBlock.hxx libcosmos/include/cosmos/error/ApiError.hxx \
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
	libcosmos/src/error/strerror.cxx libcosmos/src/formatting.cxx \
	libcosmos/src/cosmos/private/cosmos.hxx libcosmos/src/cosmos/private/strerror.hxx \
	libcosmos/src/cosmos/private/Scheduler.hxx libcosmos/src/cosmos/private/sockopts.hxx \
	libcosmos/src/cosmos/private/InfoBase.hxx libcosmos/src/cosmos/private/Initable.hxx

libxpp_la_SOURCES = libxpp/include/xpp/helpers.hxx libxpp/include/xpp/Xpp.hxx libxpp/include/xpp/Event.hxx \
	libxpp/include/xpp/types.hxx libxpp/include/xpp/Pixmap.hxx libxpp/include/xpp/SetWindowAttributes.hxx \
	libxpp/include/xpp/Property.hxx libxpp/include/xpp/XColor.hxx libxpp/include/xpp/RootWin.hxx \
	libxpp/include/xpp/XCursor.hxx libxpp/include/xpp/CachedAtom.hxx libxpp/include/xpp/PropertyTraits.hxx \
	libxpp/include/xpp/X11Exception.hxx libxpp/include/xpp/event/ClientMessageEvent.hxx libxpp/include/xpp/event/PointerMovedEvent.hxx \
	libxpp/include/xpp/event/InputEvent.hxx libxpp/include/xpp/event/ButtonEvent.hxx libxpp/include/xpp/event/KeyEvent.hxx \
	libxpp/include/xpp/event/VisibilityEvent.hxx libxpp/include/xpp/event/ConfigureEvent.hxx libxpp/include/xpp/event/FocusChangeEvent.hxx \
	libxpp/include/xpp/event/PropertyEvent.hxx libxpp/include/xpp/event/SelectionRequestEvent.hxx libxpp/include/xpp/event/SelectionEvent.hxx \
	libxpp/include/xpp/event/AnyEvent.hxx libxpp/include/xpp/event/CreateEvent.hxx \
	libxpp/include/xpp/event/SelectionClearEvent.hxx libxpp/include/xpp/event/ButtonEvent.hxx \
	libxpp/include/xpp/event/DestroyEvent.hxx libxpp/include/xpp/event/MapEvent.hxx libxpp/include/xpp/event/ReparentEvent.hxx \
	libxpp/include/xpp/WindowManagerHints.hxx libxpp/include/xpp/utils.hxx libxpp/include/xpp/SizeHints.hxx \
	libxpp/include/xpp/utf8_string.hxx libxpp/include/xpp/dso_export.h libxpp/include/xpp/AtomMapper.hxx \
	libxpp/include/xpp/XWindow.hxx libxpp/include/xpp/GraphicsContext.hxx libxpp/include/xpp/XDisplay.hxx \
	libxpp/include/xpp/formatting.hxx libxpp/include/xpp/ClassHints.hxx libxpp/include/xpp/XWindowAttrs.hxx \
	libxpp/include/xpp/keyboard.hxx libxpp/include/xpp/fwd.hxx libxpp/include/xpp/atoms.hxx \
	libxpp/src/GraphicsContext.cxx libxpp/src/XWindow.cxx libxpp/src/Pixmap.cxx \
	libxpp/src/XWindowAttrs.cxx libxpp/src/Event.cxx libxpp/src/Xpp.cxx \
	libxpp/src/PropertyTraits.cxx libxpp/src/helpers.cxx libxpp/src/XCursor.cxx \
	libxpp/src/keyboard.cxx libxpp/src/CachedAtom.cxx libxpp/src/RootWin.cxx \
	libxpp/src/XDisplay.cxx libxpp/src/AtomMapper.cxx libxpp/src/formatting.cxx \
	libxpp/src/xpp/private/Xpp.hxx

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
		main/Options.hxx main/Exception.hxx \
		main/SelectionAccessFile.hxx main/SelectionDirEntry.hxx main/SelectionOwnerFile.hxx \
		main/logger.hxx main/UpdatableDir.hxx main/WinManagerDirEntry.hxx \
		main/WinManagerFileEntry.hxx main/WindowDirEntry.hxx main/WindowFileEntry.hxx \
		main/WindowsRootDir.hxx main/Xwmfs.hxx main/main.hxx \
		main/DesktopsRootDir.hxx main/DesktopDirEntry.hxx \
		x11/WinManagerWindow.hxx common/types.hxx
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
