// libxpp
#include <xpp/atoms.hxx>

// xwmfs
#include "fuse/EventFile.hxx"
#include "main/DesktopsRootDir.hxx"
#include "main/WinManagerDirEntry.hxx"
#include "main/WinManagerFileEntry.hxx"
#include "main/Xwmfs.hxx"
#include "main/logger.hxx"
#include "x11/WinManagerWindow.hxx"

namespace xwmfs {

WinManagerDirEntry::WinManagerDirEntry(WinManagerWindow &root_win) :
		UpdateableDir{"wm", getSpecVector()}, m_root_win{root_win} {
	addEntries();

	m_events = new EventFile{*this, "events"};

	addEntry(m_events);
}

WinManagerDirEntry::SpecVector WinManagerDirEntry::getSpecVector() const {
	return SpecVector{{
		EntrySpec{"number_of_desktops",
			&WinManagerDirEntry::updateNumberOfDesktops,
			xpp::atoms::ewmh_wm_nr_desktops,
			Writable{true}},
		EntrySpec{"desktop_names",
			&WinManagerDirEntry::updateDesktopNames,
			xpp::atoms::ewmh_wm_desktop_names},
		EntrySpec{"active_desktop",
			&WinManagerDirEntry::updateActiveDesktop,
			xpp::atoms::ewmh_wm_cur_desktop,
			Writable{true}},
		EntrySpec{"active_window",
			&WinManagerDirEntry::updateActiveWindow,
			xpp::atoms::ewmh_wm_active_window,
			Writable{true}},
		EntrySpec{"show_desktop_mode",
			&WinManagerDirEntry::updateShowDesktopMode,
			xpp::atoms::ewmh_wm_desktop_shown},
		EntrySpec{"name",
			&WinManagerDirEntry::updateName,
			xpp::atoms::ewmh_window_name},
		EntrySpec{"class",
			&WinManagerDirEntry::updateClass,
			xpp::atoms::icccm_wm_class}
	}};
}

void WinManagerDirEntry::forwardEvent(const EntrySpec &changed_entry) {
	m_events->addEvent(changed_entry.name);
}

void WinManagerDirEntry::addEntries() {
	for (const auto &spec: m_specs) {
		addSpecEntry(spec);
	}
}

void WinManagerDirEntry::addSpecEntry(const EntrySpec &spec) {
	FileEntry *entry = nullptr;

	if (spec.writable)
		entry = new xwmfs::WinManagerFileEntry{spec.name};
	else
		entry = new xwmfs::FileEntry{spec.name};

	(this->*(spec.member_func))(*entry);

	*entry << '\n';

	this->addEntry(entry);
}

void WinManagerDirEntry::update(const xpp::AtomID changed_atom) {
	auto it = m_atom_update_map.find(changed_atom);

	if (it == m_atom_update_map.end()) {
		logger->warn()
			<< "Root window unknown property ("
			<< cosmos::to_integral(changed_atom) << ") changed"
			<< "\n";
		return;
	}

	const auto &update_spec = it->second;
	FileEntry *entry = getFileEntry(update_spec.name);

	if (entry) {
		logger->debug()
			<< "WinManagerDirEntry::" << __FUNCTION__
			<< ": update for " << update_spec.name << "\n";
	} else {
		logger->warn()
			<< "WinManagerDirEntry::" << __FUNCTION__
			<< "File entry " << update_spec.name
			<< " not existing?" << "\n";
		return;
	}

	this->updateModifyTime();

	entry->str("");

	try {
		(this->*(update_spec.member_func))(*entry);
		*entry << '\n';
	} catch (const std::exception &ex) {
		logger->error()
			<< "Error udpating " << update_spec.name << " property"
			<< ex.what() << "\n";
		return;
	}

	entry->setModifyTime(m_modify_time);

	forwardEvent(update_spec);
}

void WinManagerDirEntry::delProp(const xpp::AtomID deleted_atom) {
	(void)deleted_atom;
	// TODO: do something here? Is this a common use case?
}

void WinManagerDirEntry::windowLifecycleEvent(const xpp::XWindow &win,
		const bool created_else_destroyed) {
	std::string event = created_else_destroyed ? "created" : "destroyed";
	event += " ";
	event += xpp::to_string(win.id());
	m_events->addEvent(event);
}

void WinManagerDirEntry::updateNumberOfDesktops(FileEntry &entry) {
	m_root_win.fetchNumDesktops();
	entry << m_root_win.getNumDesktops();
}

void WinManagerDirEntry::updateDesktopNames(FileEntry &entry) {
	m_root_win.fetchDesktopNames();

	bool first = true;
	size_t pos;

	for (auto name: m_root_win.getDesktopNames()) {
		// protect against newlines in desktop names
		while ((pos = name.find('\n')) != name.npos) {
			name.replace(pos, 1, "\\n");
		}

		if (first)
			first = false;
		else
			entry << "\n";

		entry << name;
	}

	// update the sibling desktops directory structure
	auto desktops = Xwmfs::getInstance().getDesktopsDir();
	if (desktops) {
		desktops->handleDesktopsChanged();
	}
}

void WinManagerDirEntry::updateActiveDesktop(FileEntry &entry) {
	m_root_win.fetchActiveDesktop();
	entry << (m_root_win.hasActiveDesktop() ?
		m_root_win.getActiveDesktop() : -1);
}

void WinManagerDirEntry::updateActiveWindow(FileEntry &entry) {
	m_root_win.fetchActiveWindow();
	const auto win_id = m_root_win.hasActiveWindow() ?
		m_root_win.getActiveWindow() :
		xpp::WinID::INVALID;
	entry << xpp::to_string(win_id);
}

void WinManagerDirEntry::updateShowDesktopMode(FileEntry &entry) {
	entry << (m_root_win.hasShowDesktopMode() ?
		m_root_win.getShowDesktopMode() : -1);
}

void WinManagerDirEntry::updateName(FileEntry &entry) {
	entry << (m_root_win.hasWMName() ?
		m_root_win.getWMName() : "N/A");
}

void WinManagerDirEntry::updateClass(FileEntry &entry) {
	entry << (m_root_win.hasWMClass() ?
		m_root_win.getWMClass() : "N/A");
}

} // end ns
