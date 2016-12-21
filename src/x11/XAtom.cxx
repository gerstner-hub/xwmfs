// xwmfs
#include "x11/XAtom.hxx"
#include "main/StdLogger.hxx"

// C++
#include <cstdlib>
#include <iostream>

namespace xwmfs
{

const StandardProps& StandardProps::instance()
{
	static const StandardProps singleton;

	return singleton;
}

StandardProps::StandardProps() 
{
	try
	{
		atom_ewmh_window_name =
			XAtomMapper::getInstance().getAtom("_NET_WM_NAME");
		atom_ewmh_window_desktop =
			XAtomMapper::getInstance().getAtom("_NET_WM_DESKTOP");
		atom_ewmh_utf8_string =
			XAtomMapper::getInstance().getAtom("UTF8_STRING");
		atom_ewmh_support_check =
			XAtomMapper::getInstance().getAtom("_NET_SUPPORTING_WM_CHECK");
		atom_ewmh_wm_pid =
			XAtomMapper::getInstance().getAtom("_NET_WM_PID");
		atom_ewmh_wm_desktop_shown =
			XAtomMapper::getInstance().getAtom("_NET_SHOWING_DESKTOP");
		atom_ewmh_wm_nr_desktops =
			XAtomMapper::getInstance().getAtom("_NET_NUMBER_OF_DESKTOPS");
		atom_ewmh_wm_cur_desktop =
			XAtomMapper::getInstance().getAtom("_NET_CURRENT_DESKTOP");
		atom_ewmh_desktop_nr =
			XAtomMapper::getInstance().getAtom("_NET_WM_DESKTOP");
		atom_ewmh_wm_window_list =
			XAtomMapper::getInstance().getAtom("_NET_CLIENT_LIST");
		atom_icccm_client_machine =
			XAtomMapper::getInstance().getAtom("WM_CLIENT_MACHINE");
		atom_icccm_window_name =
			XAtomMapper::getInstance().getAtom("WM_NAME");
	}
	catch( const XDisplay::DisplayOpenError &ex )
	{
		std::cerr
			<< "Failed to populate X11 information:\n\n"
			<< ex.what()
			<< std::endl;
		std::exit( EXIT_FAILURE );
	}
}
	
XAtom XAtomMapper::getAtom(const std::string &s)
{
	XAtom ret;
	m_mappings_lock.readlock();

	AtomMapping::iterator it = m_mappings.find(s);

	if( it != m_mappings.end() )
	{
		ret = it->second;
	}
	else
	{
		m_mappings_lock.unlock();
		
		ret = XDisplay::getInstance().getAtom(s);
		
		auto &logger = xwmfs::StdLogger::getInstance();
		logger.debug() << "Resolved atom id for '" << s << "' is " << std::dec << ret.get() << std::endl;

		m_mappings_lock.writelock();

		m_mappings.insert( std::make_pair( s, (Atom)ret ) );
	}
		
	m_mappings_lock.unlock();

	return ret;
}

} // end ns

