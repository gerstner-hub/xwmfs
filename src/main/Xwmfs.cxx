// C++
#include <functional>
#include <vector>

// FUSE
#include <fuse.h>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/formatting.hxx>
#include <cosmos/fs/filesystem.hxx>
#include <cosmos/io/StreamIO.hxx>
#include <cosmos/proc/process.hxx>
#include <cosmos/proc/signal.hxx>

// libxpp
#include <xpp/atoms.hxx>
#include <xpp/event/AnyEvent.hxx>
#include <xpp/event/ConfigureEvent.hxx>
#include <xpp/event/CreateEvent.hxx>
#include <xpp/event/DestroyEvent.hxx>
#include <xpp/Event.hxx>
#include <xpp/event/MapEvent.hxx>
#include <xpp/event/PropertyEvent.hxx>
#include <xpp/event/ReparentEvent.hxx>
#include <xpp/event/SelectionClearEvent.hxx>
#include <xpp/event/SelectionEvent.hxx>
#include <xpp/event/SelectionRequestEvent.hxx>
#include <xpp/formatting.hxx>
#include <xpp/PropertyTraits.hxx>
#include <xpp/XDisplay.hxx>


// xwmfs
#include "fuse/Entry.hxx"
#include "fuse/xwmfs_fuse.hxx"
#include "main/DesktopsRootDir.hxx"
#include "main/Exception.hxx"
#include "main/logger.hxx"
#include "main/SelectionDirEntry.hxx"
#include "main/WindowsRootDir.hxx"
#include "main/WinManagerDirEntry.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs {

cosmos::FileMode Xwmfs::m_umask = cosmos::FileMode{cosmos::ModeT{0777}};

Xwmfs::Xwmfs() :
		m_display{xpp::display},
		m_root_win{m_display},
		m_ev_thread{},
		m_opts{xwmfs::Options::getInstance()} {
	m_running.store(true);
}

Xwmfs::~Xwmfs() {
	if (m_selection_window.valid()) {
		m_selection_window.destroy();
	}
	m_wm_dir = nullptr;
	m_win_dir = nullptr;
}

void Xwmfs::early_init() {
	/*
	 * To get the current umask we need to temporarily change the umask.
	 * There seems to be no better system call. Starting from Linux 4.7 an
	 * entry will be in /proc/<pid>/status, so we could read it from
	 * there, * if present. Since we know that we are single threaded at
	 * this point it is still fine to use this approach.
	 */
	m_umask = cosmos::fs::set_umask(m_umask);
	(void)cosmos::fs::set_umask(m_umask);
}

cosmos::ExitStatus Xwmfs::init() noexcept {
	auto res = cosmos::ExitStatus::SUCCESS;

	try {
		// sets the asynchronous error handlers
		::XSetErrorHandler(&Xwmfs::XErrorHandler);
		::XSetIOErrorHandler(&Xwmfs::XIOErrorHandler);

		try {
			if (m_opts.xsync()) {
				m_display.setSynchronized(true);
				logger->info() << "Operating in Xlib synchronous mode\n";
			}

			// this gets us information about newly created windows
			m_root_win.selectCreateEvent();
			// this gets us information about changed global
			// properties like the number of desktops
			m_root_win.selectPropertyNotifyEvent();
			// make sure the XServer knows about our event
			// registrations to avoid any race conditions
			m_display.sync();

			/*
			 * There is a race condition that we can't really
			 * avoid here:
			 *
			 * In createFS() we statically determine the current
			 * state of the window manager. We might already be
			 * getting events about things that happened before
			 * this initial lookup, thus artifacts like
			 * destroy events for windows we don't know about or
			 * creation events for windows we've already seen.
			 *
			 * This is probably better than the other way around:
			 * losing events for things we should have known,
			 * causing inconsistent data.
			 */

			createFS();

			createSelectionWindow();

			m_ev_thread = std::move(cosmos::PosixThread{
				{std::bind(&Xwmfs::eventThread, this)},
				"event thread"});

			setupAbortSignals(true);
		} catch (const xpp::RootWin::QueryError &ex) {
			throw Exception{cosmos::sprintf("Error querying window manager properties: %s", ex.what())};
		}
	} catch (const std::exception &ex) {
		res = cosmos::ExitStatus::FAILURE;
		logger->error() << "Error in FS operation: " << ex.what() << "\n";
	} catch(...) {
		res = cosmos::ExitStatus::FAILURE;
		logger->error() << "Error in FS operation: Unknown exception caught. Terminating.\n";
	}

	return res;
}


void Xwmfs::exit() noexcept {
	try {
		if (m_running.exchange(false)) {
			m_wakeup_event.signal();
			// finally join the thread
			m_ev_thread.join();
		}

		m_fs_root.clear();
	} catch (const std::exception &ex) {
		logger->error() << "failed to join event thread / clear file system: "
			<< ex.what() << "\n";
	}
}

void Xwmfs::updateTime() {
	m_current_time = time(nullptr);
}

void Xwmfs::createFS() {
	updateTime();

	m_fs_root.setModifyTime(m_current_time);
	m_fs_root.setStatusTime(m_current_time);

	// window manager (WM) directory that contains files with global WM information
	m_wm_dir = new xwmfs::WinManagerDirEntry{m_root_win};
	m_fs_root.addEntry(m_wm_dir);

	// now add a directory that contains a sub-directory for each window
	m_win_dir = new WindowsRootDir{};
	m_fs_root.addEntry(m_win_dir);

	// desktop / workspace specific information
	m_desktop_dir = new DesktopsRootDir{m_root_win};
	m_fs_root.addEntry(m_desktop_dir);

	// clipboard/select special files
	m_selection_dir = new xwmfs::SelectionDirEntry{};
	m_fs_root.addEntry(m_selection_dir);

	const std::vector<xpp::WinID> *windows = nullptr;

	if (m_opts.handlePseudoWindows()) {
		/*
		 * If we want to display all pseudo windows then we can't rely
		 * on the client list the window manager provides, because
		 * this only contains actual application windows.
		 *
		 * Instead we need to query the complete window tree. From
		 * there on we get events for all created windows, even pseudo
		 * ones.
		 *
		 * This is only a snapshot so there may be a race condition
		 * and we can end up with a slightly wrong initial state of
		 * windows. Not sure what to do against that.
		 */
		m_root_win.queryTree();
		windows = &(m_root_win.windowTree());
	} else {
		m_root_win.queryWindows();
		windows = &(m_root_win.windowList());
	}

	// add each window found to the file system

	{
		FileSysWriteGuard write_guard{m_fs_root};

		for (const auto &win: *windows) {
			m_win_dir->addWindow(
					xpp::XWindow{win},
					WindowsRootDir::InitialPopulation{true},
					WindowsRootDir::IsRootWin{win == m_root_win}
			);
		}
	}

	m_desktop_dir->handleDesktopsChanged();
}

void Xwmfs::createSelectionWindow() {
	m_selection_window = xpp::XWindow{m_root_win.createChild()};
	m_selection_window.setName("xwmfs selection buffer window");

	xwmfs::logger->info() << "Created selection window " << m_selection_window << "\n";
}

int Xwmfs::XErrorHandler(Display *disp, XErrorEvent *error) {
	char err_msg[512];

	::XGetErrorText(disp, error->error_code, &err_msg[0], sizeof(err_msg));

	logger->warn() << "An async X error occurred: \"" << err_msg << "\"\n";

	return 0;
}

int Xwmfs::XIOErrorHandler(Display *disp) {
	(void)disp;

	logger->error() << "A fatal async X error occurred. Exiting." << "\n";

	// perform an immediate exit, the normal exit would cause follow up
	// errors through destruction of static objects in unexpected states
	cosmos::proc::exit(cosmos::ExitStatus::FAILURE);
}

void Xwmfs::eventThread() {
	m_event_poller.create();

	/*
	 * listen on the low level XDisplay file descriptor as well as on the
	 * wakeup event (for shutdown) and the abort pipe (for aborting
	 * blocking calls).
	 */

	for (auto fd: {m_display.connectionNumber(), m_wakeup_event.fd(), m_abort_pipe.readEnd()}) {
		m_event_poller.addFD(fd, cosmos::Poller::MonitorFlag::INPUT);
	}

	while (m_running.load()) {
		try {
			auto events = m_event_poller.wait();

			for (const auto &event: events) {
				if (auto fd = event.fd(); fd == m_wakeup_event.fd()) {
					logger->info() << "Caught cancel request. Shutting down...\n";
					return;
				} else if (fd == m_abort_pipe.readEnd()) {
					readAbortPipe();
					continue;
				} else {
					// now we should be able to read at
					// least one X11 event without blocking
					handlePendingEvents();
				}
			}
		} catch (const std::exception &ex) {
			logger->error() << "unable to poll for events: " << ex.what() << "\n";
			return;
		}
	}
}

void Xwmfs::handlePendingEvents() {
	cosmos::MutexGuard g{m_event_lock};
	xpp::Event m_ev;

	/*
	 * It is important to read all pending events to avoid blocking while
	 * there are still events to be processed. This is because the
	 * Poller in the event thread only wakes up when there's network
	 * data coming in from the X server. But the libX11 can read more than
	 * one event at once from the network in one go, thus the socket might
	 * not be readable, still there would be pending events that we
	 * wouldn't process.
	 */
	while (m_display.hasPendingEvents()) {
		m_display.nextEvent(m_ev);

		try {
			// don't keep this lock for the duration of the
			// handling, because otherwise we might run into
			// cross-locking issues, because other threads may
			// hold the FS lock and want our event lock, while he
			// have the event lock but desire the FS lock.
			cosmos::MutexReverseGuard rg{m_event_lock};
			handleEvent(m_ev);
		} catch (const std::exception &ex) {
			logger->error() << "Failed to handle X11 event of type "
				<< cosmos::to_integral(m_ev.type()) << ": " << ex.what() << "\n";
		}
	}
}

void Xwmfs::handleEvent(const xpp::Event &ev) {
#if 0
	logger->debug() << "Received event #" << ev.xany.serial << " of type "
		<< std::dec << ev.type << std::endl;
#endif
	using Type = xpp::EventType;

	switch (ev.type()) {
	// a new window came into existence
	case Type::CREATE_NOTIFY: {
		const auto create_ev = xpp::CreateEvent{ev};
		if (!handleCreateEvent(create_ev)) {
			m_ignored_windows.insert(create_ev.window());
		}
		break;
	}
	// a window was destroyed
	case Type::DESTROY_NOTIFY: {
		const auto destroy_ev = xpp::DestroyEvent{ev};
		handleDestroyEvent(destroy_ev);
		auto it = m_ignored_windows.find(destroy_ev.window());
		if (it != m_ignored_windows.end()) {
			// don't need to process a window we've ignored
			// before
			m_ignored_windows.erase(it);
			break;
		}
		break;
	}
	case Type::PROPERTY_NOTIFY: {
		auto prop_ev = xpp::PropertyEvent{ev};

		logger->debug()
			<< "Property (" << prop_ev.property() << ")"
			<< " on window " << cosmos::to_integral(*prop_ev.window()) << " changed ("
			<< std::dec << cosmos::to_integral(prop_ev.state()) << ")" << std::endl;

		using Notification = xpp::PropertyNotification;

		switch (prop_ev.state()) {
		case Notification::PROPERTY_DELETE: /* FALLTHROUGH */
		case Notification::NEW_VALUE: {
			const auto is_delete = prop_ev.state() == Notification::PROPERTY_DELETE;
			xpp::XWindow win{*prop_ev.window()};
			updateTime();

			FileSysWriteGuard write_guard{m_fs_root};

			const auto prop = prop_ev.property();

			if (win == m_root_win) {
				if (is_delete)
					m_wm_dir->delProp(prop);
				else
					m_wm_dir->update(prop);
			} else {
				if (is_delete) {
					m_win_dir->deleteProperty(win, prop);
				} else {
					m_win_dir->updateProperty(win, prop);

					if (prop == xpp::atoms::ewmh_window_desktop) {
						m_desktop_dir->handleWindowDesktopChanged(win);
					}
				}
			}
			break;
		}
		default:
			break;
		}

		break;
	}
	// called upon window size/appearance changes
	case Type::CONFIGURE_NOTIFY: {
		xpp::ConfigureEvent config_ev{ev};
		xpp::XWindow w{config_ev.window()};
		updateTime();

		FileSysWriteGuard write_guard{m_fs_root};

		m_win_dir->updateGeometry(w, config_ev);
		break;
	}
	case Type::CIRCULATE_NOTIFY: {
		break;
	}
	// called if parts of the window become
	// visible/invisible
	case Type::UNMAP_NOTIFY:
	case Type::MAP_NOTIFY: {
		xpp::XWindow map_window;
		if (ev.type() == Type::MAP_NOTIFY) {
			const auto event = xpp::MapEvent{ev};
			map_window = xpp::XWindow{event.window()};
		} else {
			const auto event = xpp::UnmapEvent{ev};
			map_window = xpp::XWindow{event.window()};
		}

		if (isIgnored(map_window)) {
			break;
		}

		updateTime();
		FileSysWriteGuard write_guard{m_fs_root};
		m_win_dir->updateMappedState(map_window, ev.type() == Type::MAP_NOTIFY);
		break;
	}
	case Type::GRAVITY_NOTIFY: {
		break;
	}
	case Type::REPARENT_NOTIFY: {
		const auto reparent_ev = xpp::ReparentEvent{ev};
		xpp::XWindow w{reparent_ev.reparentedWindow()};
		w.setParent(reparent_ev.newParent());
		FileSysWriteGuard write_guard{m_fs_root};
		m_win_dir->updateParent(w);
		break;
	}
	case Type::SELECTION_NOTIFY:
	case Type::SELECTION_CLEAR:
	case Type::SELECTION_REQUEST: {
		handleSelectionEvent(ev);
		break;
	}
	default:
		logger->debug() << "Some unknown event "
			<< cosmos::to_integral(ev.type()) << " for window "
			<< xpp::XWindow{xpp::WinID{ev.toAnyEvent().window}} << " received" << "\n";
		break;
	}
}

bool Xwmfs::isPseudoWindow(const xpp::CreateEvent &ev) const {
	// Xlib manual says one should generally ignore these
	// events as they come from popups
	if (ev.overrideRedirect()) {
		logger->debug() << "Ignoring override_redirect window "
			<< cosmos::to_integral(ev.window()) << "\n";
		return true;
	} else if (ev.parent() != m_root_win.id()) {
		// This is grand-kid or something. We could add these
		// in a hierarchical manner as sub-windows but for now
		// we ignore them.
		logger->debug() << "Ignoring grand-child-window"
			<< cosmos::to_integral(ev.window()) << "\n";
		return true;
	}

	return false;
}

bool Xwmfs::handleCreateEvent(const xpp::CreateEvent &ev) {
	if (!m_opts.handlePseudoWindows() && isPseudoWindow(ev)) {
		return false;
	}

	xpp::XWindow w{ev.window()};
	w.setParent(ev.parent());

	logger->debug() << "Window " << w << " was created!" << std::endl;
	logger->debug() << "\tParent: " << xpp::XWindow{w.getParent()} << std::endl;
	logger->debug() << "\twin name = ";

	try {
		logger->debug() << w.getName() << "\n";
	} catch (const std::exception &ex) {
		logger->debug() << "<failed to get name>" << "\n";
	}

	try {
		updateTime();
		FileSysWriteGuard write_guard{m_fs_root};
		m_win_dir->addWindow(w);
		m_wm_dir->windowLifecycleEvent(w, true);
		m_desktop_dir->handleWindowCreated(w);
	} catch (const std::exception &ex) {
		logger->debug() << "\terror adding window: " << ex.what() << "\n";
	}

	return true;
}

void Xwmfs::handleDestroyEvent(const xpp::DestroyEvent &ev) {
	xpp::XWindow w{ev.window()};

	logger->debug() << "Window " << w << " was destroyed!" << "\n";

	FileSysWriteGuard write_guard{m_fs_root};
	m_win_dir->removeWindow(w);
	m_wm_dir->windowLifecycleEvent(w, false);
	m_desktop_dir->handleWindowDestroyed(w);
}

void Xwmfs::handleSelectionEvent(const xpp::Event &ev) {
	/// NOTE: the generic window() returned here might not always be what
	/// we expect? Maybe we need to inspect the concrete events.
	const xpp::XWindow w{*xpp::AnyEvent{ev}.window()};

	if (w != m_selection_window) {
		logger->warn()
			<< "Got selection buffer related event, but it's not for our selection window?"
			<< "\n";
		return;
	}

	logger->debug() << "Selection buffer event of type " << cosmos::to_integral(ev.type()) << "\n";

	if (ev.type() == xpp::EventType::SELECTION_NOTIFY) {
		// the conversion data has arrived
		m_selection_dir->conversionResult(xpp::SelectionEvent{ev});
	} else if (ev.type() == xpp::EventType::SELECTION_CLEAR) {
		// we lost ownership of the selection
		m_selection_dir->lostOwnership(xpp::SelectionClearEvent{ev});
	} else if (ev.type() == xpp::EventType::SELECTION_REQUEST) {
		// somebody wants to get the selection from us
		m_selection_dir->conversionRequest(xpp::SelectionRequestEvent{ev});
	}
}

/// Global sync signal handler for the fuse abort signal.
void fuse_abort_signal(const cosmos::Signal sig) {
	auto &xwmfs = Xwmfs::getInstance();

	const bool shutdown = sig != cosmos::signal::USR1;

	xwmfs.abortBlockingCall(shutdown);
}

void Xwmfs::setupAbortSignals(const bool on_off) {
	/*
	 * we have two troubles with implementing blocking read calls in the
	 * EventFile here:
	 *
	 * 1) when a blocking call is pending and the userspace process that
	 * blocks on it wants to interrupt the call then this situation is
	 * only made available to us via SIGUSR1. This signal will be sent by
	 * FUSE in a thread directed manner i.e. we need to setup an
	 * asynchronous signal handler and the thread this signal handler
	 * runs in is the one that needs to abort its blocking request.
	 *
	 * Our EventFile implementation uses a Condition variable for
	 * efficiently waiting for data, and the Condition variable has no way
	 * to unblock due to an asynchronously received signal (contrary to
	 * what man 7 signal says about EINTR return if SA_RESTART is not
	 * set).
	 *
	 * Thus we need to somehow keep track of which blocking calls are
	 * going on in which objects by which threads. Then the asynchronous
	 * signal handler forwards the interrupt information to a global event
	 * thread which sorts the information out and unblocks the right
	 * thread. Pretty f***ed up but that's just the way it is.
	 *
	 * https://sourceforge.net/p/fuse/mailman/message/28816288/
	 *
	 * 2) When a blocking call is pending and the FUSE userspace process
	 * gets a SIGINT or SIGTERM for shutdown then FUSE will internally
	 * deadlock.
	 *
	 * When FUSE gets the interrupt it tries to join all of its running
	 * threads before calling the fuse_exit() function. So we have no
	 * chance of knowing that we'd need to unblock the Condition wait.
	 *
	 * The way FUSE thinks we should deal with this is by using
	 * pthread_cancel and cancellation points. But this doesn't fly with us
	 * in C++ where we need to call destructors and friends.
	 *
	 * Therefore we're overwriting the SIGINT and SIGTERM signal handlers
	 * so we get a chance to abort all ongoing blocking calls. The tricky
	 * part is to forward the interrupt request to the internal FUSE
	 * routines, however.
	 *
	 * a) For this we need to know the struct fuse which is a low level
	 * data structure normally not available when we use the high level
	 * API of FUSE. We need this structure for explicitly shutting down
	 * FUSE when we intercept a SIGINT or SIGTERM.
	 *
	 * b) Another alternative is to catch the SIGINT, unblock our blocking
	 * threads and then reinstate the original SIGINT handler for FUSE to
	 * shut down the regular way.
	 *
	 * Both approaches leave room for a race condition when we've
	 * unblocked our waiting threads but before FUSE has a chance to
	 * shutdown another blocking call comes into existence. There's
	 * nothing much we can do against this, except maybe polling for
	 * blocked threads or so. Or a shutdown flag which we now have in
	 * m_shutdown.
	 *
	 * Actually the fuse_get_context() is only valid for the duration of
	 * a request call, but I hope the FUSE pointer is an exception, as
	 * this shouldn't change for the lifetime of the FUSE instance.
	 *
	 * After some testing I'm going for solution b). This works now.
	 * Solution a) had strange problems when the FUSE main thread didn't
	 * react to the fuse_exit() call on the first attempt. It seems the
	 * sem_wait() the fuse main thread does did not always return EINTR in
	 * these situations.
	 */
	cosmos::SigAction action, orig;
	action.setHandler(&fuse_abort_signal);

	for (const auto signal: {
			cosmos::signal::USR1,
			cosmos::signal::INTERRUPT,
			cosmos::signal::TERMINATE}) {

		try {
			cosmos::signal::set_action(signal, on_off ? action : m_signal_handlers[signal], &orig);
		} catch (const std::exception &ex) {
			throw Exception{cosmos::sprintf(
					"Failed to setup abort signal handlers: %s", ex.what())};
		}

		if (on_off) {
			// save the original signal handle to restore it later
			m_signal_handlers[signal] = orig;
		}
	}
}

void Xwmfs::abortBlockingCall(const bool all) {
	/*
	 * Send the ID of the thread that got the signal over the abort pipe
	 * so that the event thread can deal with the situation without
	 * restrictions of async signal handling
	 */

	AbortMsg msg;
	msg.type = all ? AbortType::SHUTDOWN : AbortType::CALL;
	msg.thread = cosmos::pthread::get_id().raw();

	auto pipe_fd = m_abort_pipe.writeEnd();
	cosmos::StreamIO pipe_io{pipe_fd};

	try {
		// writes of <= PIPE_BUF bytes are atomic so we don't need to
		// fear partial writes here. But the call may block if the
		// pipe is full.
		pipe_io.writeAll(&msg, sizeof(msg));
	} catch (const std::exception &ex) {
		logger->error() << "failed to write to abort pipe: " << ex.what() << "\n";
	}
}

void Xwmfs::abortBlockingCall(const cosmos::pthread::ID thread) {
	cosmos::MutexGuard g{m_blocking_call_lock};

	auto it = m_blocking_calls.find(thread);

	if (it == m_blocking_calls.end()) {
		logger->error() << "Failed to find abort entry for thread" << std::endl;
		return;
	}

	logger->info() << "Abort request for some blocking call" << std::endl;

	auto ef = it->second;

	ef->abortBlockingCall(thread);
}

void Xwmfs::abortAllBlockingCalls() {
	cosmos::MutexGuard g{m_blocking_call_lock};

	for (auto it: m_blocking_calls) {
		auto entry = it.second;

		entry->abortBlockingCall(it.first);
	}

	/*
	 * This signaling flag is necessary to avoid race conditions: all
	 * blocking calls may return but userspace programs often react to
	 * EINTR by retrying the blocking calls, which would cause again a
	 * deadlock. This flag helps us to continuously return EINTR to
	 * successive calls.
	 */
	m_shutdown = true;
}

void Xwmfs::readAbortPipe() {
	AbortMsg msg;
	auto pipe_fd = m_abort_pipe.readEnd();
	cosmos::StreamIO pipe_io{pipe_fd};

	// reads <= PIPE_BUF bytes from the pipe are atomic, shouldn't even block.
	try {
		pipe_io.readAll(reinterpret_cast<void*>(&msg), sizeof(msg));
	} catch (const std::exception &ex) {
		logger->error() << "Failed to read from abort pipe: " << ex.what() << "\n";
		return;
	}

	if (msg.type == AbortType::CALL) {
		abortBlockingCall(cosmos::pthread::ID{msg.thread});
	} else {
		abortAllBlockingCalls();
		/* reinstate original signal handlers */
		setupAbortSignals(false);
		cosmos::signal::send(cosmos::proc::get_own_pid(), cosmos::signal::INTERRUPT);
	}
}

bool Xwmfs::registerBlockingCall(Entry *entry) {
	cosmos::MutexGuard g{m_blocking_call_lock};

	if (m_shutdown) {
		return false;
	}

	m_blocking_calls.insert(std::make_pair(cosmos::pthread::get_id(), entry));

	return true;
}

void Xwmfs::unregisterBlockingCall() {
	cosmos::MutexGuard g(m_blocking_call_lock);

	m_blocking_calls.erase(cosmos::pthread::get_id());
}

} // end ns
