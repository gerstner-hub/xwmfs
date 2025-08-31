// C++
#include <iostream>
#include <sstream>

// libxpp
#include <xpp/AtomMapper.hxx>
#include <xpp/atoms.hxx>
#include <xpp/event/ConfigureEvent.hxx>
#include <xpp/helpers.hxx>
#include <xpp/Property.hxx>
#include <xpp/X11Exception.hxx>
#include <xpp/XWindowAttrs.hxx>

// xwmfs
#include "fuse/EventFile.hxx"
#include "main/Exception.hxx"
#include "main/logger.hxx"
#include "main/WindowDirEntry.hxx"
#include "main/WindowFileEntry.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs {

WindowDirEntry::WindowDirEntry(const xpp::XWindow &win, const bool query_attrs) :
		UpdateableDir{xpp::to_string(win.id()), getSpecVector()},
		m_win{win} {
	addEntries();

	m_events = new EventFile{*this, "events"};
	addEntry(m_events);

	m_mapped = new WindowFileEntry{"mapped",
		m_win, m_modify_time, Writable{false}};
	addEntry(m_mapped);

	m_geometry = new WindowFileEntry{"geometry",
		m_win, m_modify_time};
	addEntry(m_geometry);
	{
		xpp::XWindowAttrs attrs;
		try {
			m_win.getAttrs(attrs);
			updateGeometry(attrs);
		} catch (const xpp::X11Exception &ex) {
			// window disappeared again?
		}
	}

	// NOTE: might become a writeable entry, using XReparentWindow(),
	// pretty obscure though
	m_parent = new WindowFileEntry{"parent",
		m_win, m_modify_time, Writable{false}};
	addEntry(m_parent);
	try {
		m_win.updateFamily();
	} catch (const xpp::X11Exception &ex) {
		// window disappeared again?
	}
	updateParent();

	if (query_attrs) {
		queryAttrs();
	} else {
		setDefaultAttrs();
	}
}

void WindowDirEntry::addEntries() {
	for (const auto &spec: m_specs) {
		addSpecEntry(spec);
	}
}

bool WindowDirEntry::markDeleted() {
	m_events->addEvent("destroyed");

	return UpdateableDir::markDeleted();
}

WindowDirEntry::SpecVector WindowDirEntry::getSpecVector() const {
	return SpecVector{{
		EntrySpec{"id", &WindowDirEntry::updateId},
		EntrySpec{"name", &WindowDirEntry::updateWindowName, {
				xpp::atoms::icccm_window_name,
				xpp::atoms::ewmh_window_name
			}, Writable{true}
		},
		EntrySpec{"desktop", &WindowDirEntry::updateDesktop,
			xpp::atoms::ewmh_desktop_nr,
			Writable{true}},
		EntrySpec{"pid", &WindowDirEntry::updatePID,
			xpp::atoms::ewmh_wm_pid},
		EntrySpec{"control", &WindowDirEntry::updateCommandControl,
			Writable{true}},
		EntrySpec{"client_machine",
			&WindowDirEntry::updateClientMachine},
		EntrySpec{"properties",
			&WindowDirEntry::updateProperties,
			Writable{true},
			AlwaysUpdate{true}},
		EntrySpec{"class", &WindowDirEntry::updateClass,
			xpp::atoms::icccm_wm_class
		},
		EntrySpec{"command", &WindowDirEntry::updateCommand,
			xpp::atoms::icccm_wm_command},
		EntrySpec{"locale", &WindowDirEntry::updateLocale,
			xpp::atoms::icccm_wm_locale},
		EntrySpec{"protocols", &WindowDirEntry::updateProtocols,
			xpp::atoms::icccm_wm_protocols},
		EntrySpec{"client_leader", &WindowDirEntry::updateClientLeader,
			xpp::atoms::icccm_wm_client_leader},
		EntrySpec{"window_type", &WindowDirEntry::updateWindowType,
			xpp::atoms::ewmh_wm_window_type}
	}};
}

void WindowDirEntry::addSpecEntry(
		const UpdateableDir<WindowDirEntry>::EntrySpec &spec) {
	FileEntry *entry = new xwmfs::WindowFileEntry{
		spec.name, m_win, m_modify_time, spec.writable
	};

	try {
		(this->*(spec.member_func))(*entry);
	} catch (const std::exception &ex) {
		// this can happen legally. It is a race condition. We've been
		// so fast to register the window but it hasn't got a name or
		// whatever property yet.
		//
		// The name will be noticed later on via a property update.
		xwmfs::logger->debug()
			<< "Couldn't get " << spec.name
			<< " for window " << xpp::to_string(m_win.id())
			<< " right away" << std::endl;
		delete entry;
		return;
	}

	*entry << '\n';

	this->addEntry(entry, false);
}


std::string WindowDirEntry::getCommandInfo() {
	std::string ret;

	for (const auto command: { "destroy", "delete" }) {
		// provide the available commands as read content
		ret += command;
		ret += " ";
	}

	ret.pop_back();

	return ret;
}

void WindowDirEntry::updateAll() {
	for (auto it: m_atom_update_map) {
		update(it.first);
	}
}

void WindowDirEntry::propertyChanged(const xpp::AtomID changed_atom, const bool is_delete) {
	// do the same for delete and update at the moment
	// upon delete empty files might remain in the process of updating
	// them. Removal of those files is a TODO
	(void)is_delete;
	auto it = m_atom_update_map.find(changed_atom);

	if (it != m_atom_update_map.end()) {
		update(it->second);
	}

	for (const auto &spec: m_always_update_specs) {
		update(spec);
	}
}

void WindowDirEntry::update(const EntrySpec &spec) {
	updateModifyTime();
	const auto entry = this->getFileEntry(spec.name);

	// the property was not available during window creation but now here
	// it is
	if (!entry) {
		addSpecEntry(spec);
		return;
	}

	try {
		entry->str("");
		(this->*(spec.member_func))(*entry);
		(*entry) << '\n';
	} catch(...) {
		xwmfs::logger->error()
			<< "Error udpating property '" << spec.name << "'"
			<< std::endl;
	}

	entry->setModifyTime(m_modify_time);

	forwardEvent(spec);
}

void WindowDirEntry::newMappedState(const bool mapped) {
	m_mapped->str("");
	(*m_mapped) << (mapped ? "1" : "0") << "\n";

	m_events->addEvent("mapped");
}

void WindowDirEntry::newGeometry(const xpp::ConfigureEvent &event) {
	xpp::XWindowAttrs attrs;
	const auto spec = event.spec();
	attrs.x = spec.x;
	attrs.y = spec.y;
	attrs.width = spec.width;
	attrs.height = spec.height;
	updateGeometry(attrs);
	m_events->addEvent("geometry");
}

void WindowDirEntry::forwardEvent(const EntrySpec &changed_entry) {
	m_events->addEvent(changed_entry.name);
}

void WindowDirEntry::updateWindowName(FileEntry &entry) {
	entry << m_win.getName();
}

void WindowDirEntry::updateDesktop(FileEntry &entry) {
	entry << m_win.getDesktop();
}

void WindowDirEntry::updateId(FileEntry &entry) {
	entry << xpp::to_string(m_win.id());
}

void WindowDirEntry::updatePID(FileEntry &entry) {
	entry << cosmos::to_integral(m_win.getPID());
}

void WindowDirEntry::updateCommand(FileEntry &entry) {
	entry << m_win.getCommand();
}

void WindowDirEntry::updateLocale(FileEntry &entry) {
	entry << m_win.getLocale();
}

void WindowDirEntry::updateProtocols(FileEntry &entry) {
	xpp::AtomIDVector prots;

	m_win.getProtocols(prots);

	const auto &mapper = xpp::atom_mapper;
	bool first = true;

	for (const auto &atom: prots) {
		entry << (first ? "" : "\n") << mapper.mapName(atom);
		first = false;
	}
}

void WindowDirEntry::updateClientLeader(FileEntry &entry) {
	auto leader = m_win.getClientLeader();

	entry << xpp::to_string(leader);
}

void WindowDirEntry::updateWindowType(FileEntry &entry) {
	const auto _type = m_win.getWindowType();

	entry << xpp::atom_mapper.mapName(_type);
}

void WindowDirEntry::updateGeometry(const xpp::XWindowAttrs &attrs) {
	m_geometry->str("");
	(*m_geometry) << attrs.x << "," << attrs.y << ":" << attrs.width << "x" << attrs.height << "\n";
}

void WindowDirEntry::updateCommandControl(FileEntry &entry) {
	entry << getCommandInfo();
}

void WindowDirEntry::updateClientMachine(FileEntry &entry) {
	entry << m_win.getClientMachine();
}

namespace {

void getPropertyValue(const xpp::XWindow &win, const xpp::AtomID prop_atom,
		const xpp::XWindow::PropertyInfo &info, std::stringstream &value) {
	/*
	 * this code could be more compact via templates but would then also
	 * be more complex ...
	 */
	using Atom = xpp::AtomID;

	switch (info.type) {
		case Atom::ATOM: {
			xpp::Property<std::vector<xpp::AtomID>> prop;
			win.getProperty(prop_atom, prop, &info);
			int i = 0;
			for (const auto &val: prop.get()) {
				const auto &name = xpp::atom_mapper.mapName(val);
				if (i++)
					value << " ";
				value << name;
			}
			break;
		}
		case Atom::CARDINAL: {
			if (info.items == 1) {
				xpp::Property<int> prop;
				win.getProperty(prop_atom, prop, &info);
				value << prop.get();
			} else {
				xpp::Property<std::vector<int>> prop;
				win.getProperty(prop_atom, prop, &info);
				for (const auto &val: prop.get()) {
					value << val << " ";
				}
			}
			break;
		}
		case Atom::STRING: {
			xpp::Property<const char*> prop;
			win.getProperty(prop_atom, prop, &info);
			value << prop.get();
			break;
		}
		case Atom::WINDOW: {
			xpp::Property<xpp::WinID> prop;
			win.getProperty(prop_atom, prop, &info);
			value << xpp::to_string(prop.get());
			break;
		}
		default: {
			if (info.type == xpp::atoms::ewmh_utf8_string) {
				xpp::Property<xpp::utf8_string> prop;
				win.getProperty(prop_atom, prop, &info);
				value << prop.get().str;
			} else {
				// some unknown property type, display as hex
				// TODO
			}

			break;
		}
	}
}

} // end anon ns

void WindowDirEntry::updateProperties(FileEntry &entry) {
	xpp::AtomIDVector atoms;
	m_win.getPropertyList(atoms);

	bool first = true;
	xpp::XWindow::PropertyInfo info;

	for (const auto atom: atoms) {
		m_win.getPropertyInfo(atom, info);
		const auto &name = xpp::atom_mapper.mapName(atom);
		const auto &_type = xpp::atom_mapper.mapName(info.type);

		logger->debug()
			<< "Querying property " << cosmos::to_integral(atom) << " on window "
			<< xpp::to_string(m_win) << std::endl;
		logger->debug()
			<< "type = " << cosmos::to_integral(info.type) << ", items = " << info.items
			<< ", format = " << info.format << std::endl;

		entry << (first ? "" : "\n") << name << "(" << _type << ") = ";

		try {
			getPropertyValue(m_win, atom, info, entry);
		} catch (const std::exception &ex) {
			logger->error()
				<< "Error getting property value for "
				<< xpp::to_string(m_win.id()) << "/" << cosmos::to_integral(atom)
				<< ": " << ex.what() << std::endl;
			entry << "<error>";
		}
		first = false;
	}
}

void WindowDirEntry::updateClass(FileEntry &entry) {
	const auto &class_pair = m_win.getClass();
	entry << class_pair.first << "\n" << class_pair.second;
}

void WindowDirEntry::queryAttrs() {
	xpp::XWindowAttrs attrs;
	try {
		m_win.getAttrs(attrs);

		newMappedState(attrs.isMapped());
	} catch (const std::exception &ex) {
		xwmfs::logger->error()
			<< "Error getting window attrs for " << xpp::to_string(m_win.id())
			<< ": " << ex.what() << std::endl;
		setDefaultAttrs();
	}
}

void WindowDirEntry::setDefaultAttrs() {
	(*m_mapped) << "0\n";
}

void WindowDirEntry::updateParent() {
	m_parent->str("");
	(*m_parent) << xpp::to_string(m_win.getParent()) << "\n";
}

void WindowDirEntry::newParent(const xpp::XWindow &win) {
	m_events->addEvent("parent");
	m_win.setParent(win);

	updateParent();
}

} // end ns
