// xwmfs
#include "main/WinManagerDirEntry.hxx"
#include "main/WinManagerFileEntry.hxx"
#include "main/StdLogger.hxx"
#include "x11/RootWin.hxx"

namespace xwmfs
{

WinManagerDirEntry::WinManagerDirEntry(RootWin &root_win) :
	UpdateableDir("wm", getSpecVector()),
	m_root_win(root_win)
{
	addEntries();
}

WinManagerDirEntry::SpecVector WinManagerDirEntry::getSpecVector() const
{
	auto std_props = StandardProps::instance();

	return SpecVector( {
		EntrySpec("number_of_desktops", &WinManagerDirEntry::updateNumberOfDesktops, true,
			std_props.atom_ewmh_wm_nr_desktops),
		EntrySpec("active_desktop", &WinManagerDirEntry::updateActiveDesktop, true,
			std_props.atom_ewmh_wm_cur_desktop),
		EntrySpec("active_window", &WinManagerDirEntry::updateActiveWindow, true,
			std_props.atom_ewmh_wm_active_window),
		EntrySpec("show_desktop_mode", &WinManagerDirEntry::updateShowDesktopMode, false,
			std_props.atom_ewmh_wm_desktop_shown),
		EntrySpec("name", &WinManagerDirEntry::updateName, false,
			std_props.atom_ewmh_window_name),
		EntrySpec("class", &WinManagerDirEntry::updateClass, false,
			XAtom(XA_WM_CLASS))
	} );
}

void WinManagerDirEntry::addEntries()
{
	for( const auto &spec: m_specs )
	{
		addSpecEntry(spec);
	}
}

void WinManagerDirEntry::addSpecEntry(
	const UpdateableDir<WinManagerDirEntry>::EntrySpec &spec
)
{
	FileEntry *entry = nullptr;

	if( spec.read_write )
		entry = new xwmfs::WinManagerFileEntry(spec.name);
	else
		entry = new xwmfs::FileEntry(spec.name, false);

	(this->*(spec.member_func))(*entry);

	*entry << '\n';

	this->addEntry(entry);
}

void WinManagerDirEntry::update(const Atom changed_atom)
{
	auto &logger = xwmfs::StdLogger::getInstance();
	auto it = m_atom_update_map.find(XAtom(changed_atom));

	if( it == m_atom_update_map.end() )
	{
		logger.warn()
			<< "Root window unknown property "
			<< "0x" << changed_atom << ") changed" << std::endl;
		return;
	}

	const auto &update_spec = it->second;
	FileEntry *entry = getFileEntry(update_spec.name);

	if( entry )
	{
		logger.debug()
			<< "WinManagerDirEntry::" << __FUNCTION__
			<< ": update for " << update_spec.name << std::endl;
	}
	else
	{
		logger.warn()
			<< "File entry " << update_spec.name
			<< " not existing?" << std::endl;
		return;
	}

	this->updateModifyTime();

	entry->str("");

	try
	{
		(this->*(update_spec.member_func))(*entry);
		(*entry) << '\n';
	}
	catch(...)
	{
		logger.error()
			<< "Error udpating " << update_spec.name << " property"
			<< std::endl;
		return;
	}

	entry->setModifyTime(m_modify_time);
}

void WinManagerDirEntry::updateNumberOfDesktops(FileEntry &entry)
{
	m_root_win.updateNumberOfDesktops();
	entry << m_root_win.getWM_NumDesktops();
}

void WinManagerDirEntry::updateActiveDesktop(FileEntry &entry)
{
	m_root_win.updateActiveDesktop();
	entry << (m_root_win.hasWM_ActiveDesktop() ?
		m_root_win.getWM_ActiveDesktop() : -1);
}

void WinManagerDirEntry::updateActiveWindow(FileEntry &entry)
{
	m_root_win.updateActiveWindow();
	entry << (m_root_win.hasWM_ActiveWindow() ?
		m_root_win.getWM_ActiveWindow() : Window(0));
}

void WinManagerDirEntry::updateShowDesktopMode(FileEntry &entry)
{
	entry << (m_root_win.hasWM_ShowDesktopMode() ? m_root_win.getWM_ShowDesktopMode() : -1);
}

void WinManagerDirEntry::updateName(FileEntry &entry)
{
	entry << (m_root_win.hasWM_Name() ? m_root_win.getWM_Name() : "N/A");
}

void WinManagerDirEntry::updateClass(FileEntry &entry)
{
	entry << (m_root_win.hasWM_Class() ?  m_root_win.getWM_Class() : "N/A");
}

} // end ns

