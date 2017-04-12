// C++

// xwmfs
#include "main/WindowDirEntry.hxx"
#include "main/WindowFileEntry.hxx"
#include "main/StdLogger.hxx"
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

	m_mapped = new  WindowFileEntry("mapped", m_win);
	addEntry(m_mapped);

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
		EntrySpec("command", &WindowDirEntry::updateCommandControl, true),
		EntrySpec("client_machine", &WindowDirEntry::updateClientMachine, false)
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

void WindowDirEntry::update(Atom changed_atom)
{
	auto it = m_atom_update_map.find(XAtom(changed_atom));

	if( it == m_atom_update_map.end() )
	{
		return;
	}

	updateModifyTime();
	const auto &spec = it->second;
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

void WindowDirEntry::updateCommandControl(FileEntry &entry)
{
	entry << getCommandInfo();
}

void WindowDirEntry::updateClientMachine(FileEntry &entry)
{
	entry << m_win.getClientMachine();
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

} // end ns

