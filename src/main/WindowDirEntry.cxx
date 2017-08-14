// C++
#include <sstream>
#include <iostream>

// xwmfs
#include "main/WindowDirEntry.hxx"
#include "main/WindowFileEntry.hxx"
#include "main/StdLogger.hxx"
#include "main/Xwmfs.hxx"
#include "x11/XAtom.hxx"
#include "x11/XWindowAttrs.hxx"
#include "fuse/EventFile.hxx"

namespace xwmfs
{

WindowDirEntry::WindowDirEntry(const XWindow &win, const bool query_attrs) :
	UpdateableDir(win.idStr(), getSpecVector()),
	m_win(win)
{
	addEntries();

	m_events = new EventFile(*this, "events");
	addEntry(m_events);

	m_mapped = new WindowFileEntry("mapped", m_win, m_modify_time, false);
	addEntry(m_mapped);

	m_geometry = new WindowFileEntry("geometry", m_win, m_modify_time, false);
	addEntry(m_geometry);
	{
		XWindowAttrs attrs;
		m_win.getAttrs(attrs);
		updateGeometry(attrs);
	}

	// NOTE: might become a writeable entry, using XReparentWindow(),
	// pretty obscure though
	m_parent = new WindowFileEntry("parent", m_win, m_modify_time, false);
	addEntry(m_parent);
	m_win.updateFamily();
	updateParent();

	if( query_attrs )
	{
		queryAttrs();
	}
	else
	{
		setDefaultAttrs();
	}
}

void WindowDirEntry::addEntries()
{
	for( const auto &spec: m_specs )
	{
		addSpecEntry(spec);
	}
}

bool WindowDirEntry::markDeleted()
{
	m_events->addEvent("destroyed");

	return UpdateableDir::markDeleted();
}

WindowDirEntry::SpecVector WindowDirEntry::getSpecVector() const
{
	auto std_props = StandardProps::instance();

	return SpecVector( {
		EntrySpec("id", &WindowDirEntry::updateId, false),
		EntrySpec("name", &WindowDirEntry::updateWindowName, true,
			{
				std_props.atom_icccm_window_name,
				std_props.atom_ewmh_window_name
			}
		),
		EntrySpec("desktop", &WindowDirEntry::updateDesktop, true,
			std_props.atom_ewmh_desktop_nr),
		EntrySpec("pid", &WindowDirEntry::updatePID, false,
			std_props.atom_ewmh_wm_pid),
		EntrySpec("control", &WindowDirEntry::updateCommandControl, true),
		EntrySpec("client_machine", &WindowDirEntry::updateClientMachine, false),
		EntrySpec("properties", &WindowDirEntry::updateProperties, false, true /* always update this entry */),
		EntrySpec("class", &WindowDirEntry::updateClass, false,
			std_props.atom_icccm_wm_class
		),
		EntrySpec("command", &WindowDirEntry::updateCommand, false,
			std_props.atom_icccm_wm_command),
		EntrySpec("locale", &WindowDirEntry::updateLocale, false,
			std_props.atom_icccm_wm_locale),
		EntrySpec("protocols", &WindowDirEntry::updateProtocols, false,
			std_props.atom_icccm_wm_protocols),
		EntrySpec("client_leader", &WindowDirEntry::updateClientLeader, false,
			std_props.atom_icccm_wm_client_leader),
		EntrySpec("window_type", &WindowDirEntry::updateWindowType, false,
			std_props.atom_ewmh_wm_window_type)
	} );
}

void WindowDirEntry::addSpecEntry(
	const UpdateableDir<WindowDirEntry>::EntrySpec &spec
)
{
	FileEntry *entry = new xwmfs::WindowFileEntry(
		spec.name, m_win, m_modify_time, spec.read_write
	);

	try
	{
		(this->*(spec.member_func))(*entry);
	}
	catch( const xwmfs::Exception &ex )
	{
		// this can happen legally. It is a race condition. We've been
		// so fast to register the window but it hasn't got a name or
		// whatever property yet.
		//
		// The name will be noticed later on via a property update.
		xwmfs::StdLogger::getInstance().debug()
			<< "Couldn't get " << spec.name
			<< " for window " << m_win.id()
			<< " right away" << std::endl;
		delete entry;
		return;
	}

	*entry << '\n';

	this->addEntry(entry, false);
}


std::string WindowDirEntry::getCommandInfo()
{
	std::string ret;

	for( const auto command: { "destroy", "delete" } )
	{
		// provide the available commands as read content
		ret += command;
		ret += " ";
	}

	ret.pop_back();

	return ret;
}

void WindowDirEntry::updateAll()
{
	for( auto it: m_atom_update_map )
	{
		update(it.first);
	}
}

void WindowDirEntry::update(Atom changed_atom)
{
	auto it = m_atom_update_map.find(XAtom(changed_atom));

	if( it != m_atom_update_map.end() )
	{
		update(it->second);
	}

	for( const auto &spec: m_always_update_specs )
	{
		update(spec);
	}
}

void WindowDirEntry::update(const EntrySpec &spec)
{
	updateModifyTime();
	const auto entry = this->getFileEntry(spec.name);

	// the property was not available during window creation but now here
	// it is
	if( ! entry )
	{
		addSpecEntry(spec);
		return;
	}

	try
	{
		entry->str("");
		(this->*(spec.member_func))(*entry);
		(*entry) << '\n';
	}
	catch(...)
	{
		xwmfs::StdLogger::getInstance().error()
			<< "Error udpating property '" << spec.name << "'"
			<< std::endl;
	}

	entry->setModifyTime(m_modify_time);

	forwardEvent(spec);
}

void WindowDirEntry::newMappedState(const bool mapped)
{
	m_mapped->str("");
	(*m_mapped) << (mapped ? "1" : "0") << "\n";

	m_events->addEvent("mapped");
}

void WindowDirEntry::newGeometry(const XConfigureEvent &event)
{
	XWindowAttrs attrs;
	attrs.x = event.x;
	attrs.y = event.y;
	attrs.width = event.width;
	attrs.height = event.height;
	updateGeometry(attrs);
	m_events->addEvent("geometry");
}

void WindowDirEntry::forwardEvent(const EntrySpec &changed_entry)
{
	m_events->addEvent(changed_entry.name);
}

void WindowDirEntry::updateWindowName(FileEntry &entry)
{
	entry << m_win.getName();
}

void WindowDirEntry::updateDesktop(FileEntry &entry)
{
	entry << m_win.getDesktop();
}

void WindowDirEntry::updateId(FileEntry &entry)
{
	entry << m_win.idStr();
}

void WindowDirEntry::updatePID(FileEntry &entry)
{
	entry << m_win.getPID();
}

void WindowDirEntry::updateCommand(FileEntry &entry)
{
	entry << m_win.getCommand();
}

void WindowDirEntry::updateLocale(FileEntry &entry)
{
	entry << m_win.getLocale();
}

void WindowDirEntry::updateProtocols(FileEntry &entry)
{
	XWindow::AtomVector prots;

	m_win.getProtocols(prots);

	const auto &mapper = XAtomMapper::getInstance();
	bool first = true;

	for( const auto &atom: prots )
	{
		entry << (first ? "" : "\n") << mapper.getName(XAtom(atom));
		first = false;
	}
}

void WindowDirEntry::updateClientLeader(FileEntry &entry)
{
	auto leader = m_win.getClientLeader();

	entry << leader;
}

void WindowDirEntry::updateWindowType(FileEntry &entry)
{
	auto type = m_win.getWindowType();

	const auto &mapper = XAtomMapper::getInstance();

	entry << mapper.getName(XAtom(type));
}

void WindowDirEntry::updateGeometry(const XWindowAttrs &attrs)
{
	m_geometry->str("");
	(*m_geometry) << attrs.x << "," << attrs.y << "," << attrs.width << "," << attrs.height << "\n";
}

void WindowDirEntry::updateCommandControl(FileEntry &entry)
{
	entry << getCommandInfo();
}

void WindowDirEntry::updateClientMachine(FileEntry &entry)
{
	entry << m_win.getClientMachine();
}

namespace
{

void getPropertyValue(
	const XWindow &win,
	const XAtom &prop_atom,
	const XWindow::PropertyInfo &info,
	std::stringstream &value
)
{
	/*
	 * this code could be more compact via templates but would then also
	 * be more complex ...
	 */
	auto std_props = StandardProps::instance();

	switch(info.type)
	{
		case XA_CARDINAL:
		{
			if( info.items == 1 )
			{
				Property<int> prop;
				win.getProperty(prop_atom, prop, &info);
				value << prop.get();
			}
			else
			{
				Property<std::vector<int>> prop;
				win.getProperty(prop_atom, prop, &info);
				for( const auto &val: prop.get() )
				{
					value << val << " ";
				}
			}
			break;
		}
		case XA_STRING:
		{
			Property<const char*> prop;
			win.getProperty(prop_atom, prop, &info);
			value << prop.get();
			break;
		}
		case XA_WINDOW:
		{
			Property<Window> prop;
			win.getProperty(prop_atom, prop, &info);
			value << prop.get();
			break;
		}
		default:
		{
			if( info.type == std_props.atom_ewmh_utf8_string )
			{
				Property<utf8_string> prop;
				win.getProperty(prop_atom, prop, &info);
				value << prop.get().data;
			}
			else
			{
				// some unknown property type, display as hex
				// TODO
			}

			break;
		}
	}
}

std::string getAtomTypeLabel(const XWindow::PropertyInfo &info)
{
	auto std_props = StandardProps::instance();

	switch( info.type )
	{
	case XA_CARDINAL:
		return "I";
	case XA_STRING:
		return "S";
	case XA_WINDOW:
		return "W";
	}

	if( info.type == std_props.atom_ewmh_utf8_string )
		return "U";

	return "?";
}

} // end ns

void WindowDirEntry::updateProperties(FileEntry &entry)
{
	auto &logger = xwmfs::StdLogger::getInstance();
	const auto &mapper = XAtomMapper::getInstance();
	XWindow::AtomVector atoms;
	m_win.getPropertyList(atoms);

	bool first = true;
	XWindow::PropertyInfo info;

	for( const auto &plain_atom: atoms )
	{
		const XAtom atom(plain_atom);
		m_win.getPropertyInfo(atom, info);
		const auto &name = mapper.getName(atom);

		logger.debug()
			<< "Querying property " << atom << " on window "
			<< m_win << std::endl;
		logger.debug()
			<< "type = " << info.type << ", items = " << info.items << ", format = " << info.format << std::endl;

		entry << (first ? "" : "\n") << name << "(" << getAtomTypeLabel(info) << ") = ";

		try
		{
			getPropertyValue(m_win, atom, info, entry);
		}
		catch( const Exception &ex )
		{
			logger.error()
				<< "Error getting property value for "
				<< m_win << "/" << atom << ": " << ex.what()
				<< std::endl;
			entry << "<error>";
		}
		first = false;
	}
}

void WindowDirEntry::updateClass(FileEntry &entry)
{
	const auto &class_pair = m_win.getClass();
	entry << class_pair.first << "\n" << class_pair.second;
}

void WindowDirEntry::queryAttrs()
{
	XWindowAttrs attrs;
	try
	{
		m_win.getAttrs(attrs);

		newMappedState(attrs.isMapped());
	}
	catch( const xwmfs::Exception &ex )
	{
		xwmfs::StdLogger::getInstance().error()
			<< "Error getting window attrs for " << m_win << ": " << ex.what()
			<< std::endl;
		setDefaultAttrs();
	}
}

void WindowDirEntry::setDefaultAttrs()
{
	(*m_mapped) << "0\n";
}

void WindowDirEntry::updateParent()
{
	m_parent->str("");
	(*m_parent) << m_win.getParent() << "\n";
}

void WindowDirEntry::newParent(const XWindow &win)
{
	m_events->addEvent("parent");
	m_win.setParent(win);

	updateParent();
}

} // end ns

