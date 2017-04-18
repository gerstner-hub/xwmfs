// xwmfs
#include "x11/XAtom.hxx"
#include "main/StdLogger.hxx"

// C++
#include <cstdlib>
#include <iostream>

namespace xwmfs
{

XAtomMapper& XAtomMapper::getInstance()
{
	static XAtomMapper inst;

	return inst;
}

const StandardProps& StandardProps::instance()
{
	static const StandardProps singleton;

	return singleton;
}

StandardProps::StandardProps() 
{
	try
	{
		auto &mapper = XAtomMapper::getInstance();
		atom_ewmh_window_name = mapper.getAtom("_NET_WM_NAME");
		atom_ewmh_window_desktop = mapper.getAtom("_NET_WM_DESKTOP");
		atom_ewmh_window_pid = mapper.getAtom("_NET_WM_PID");
		atom_ewmh_utf8_string = mapper.getAtom("UTF8_STRING");
		atom_ewmh_support_check = mapper.getAtom("_NET_SUPPORTING_WM_CHECK");
		atom_ewmh_wm_pid = mapper.getAtom("_NET_WM_PID");
		atom_ewmh_wm_desktop_shown = mapper.getAtom("_NET_SHOWING_DESKTOP");
		atom_ewmh_wm_nr_desktops = mapper.getAtom("_NET_NUMBER_OF_DESKTOPS");
		atom_ewmh_wm_cur_desktop = mapper.getAtom("_NET_CURRENT_DESKTOP");
		atom_ewmh_desktop_nr = mapper.getAtom("_NET_WM_DESKTOP");
		atom_ewmh_wm_window_list = mapper.getAtom("_NET_CLIENT_LIST");
		atom_ewmh_wm_active_window = mapper.getAtom("_NET_ACTIVE_WINDOW");
		atom_icccm_client_machine = mapper.getAtom("WM_CLIENT_MACHINE");
		atom_icccm_window_name = mapper.getAtom("WM_NAME");
		atom_icccm_wm_protocols = mapper.getAtom("WM_PROTOCOLS");
		atom_icccm_wm_delete_window = mapper.getAtom("WM_DELETE_WINDOW");
		atom_icccm_wm_client_machine = mapper.getAtom("WM_CLIENT_MACHINE");
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
	{
		ReadLockGuard g(m_mappings_lock);

		AtomMapping::iterator it = m_mappings.find(s);

		if( it != m_mappings.end() )
		{
			return XAtom(it->second);
		}
	}

	return cacheMiss(s);
}

const std::string& XAtomMapper::getName(const XAtom &atom) const
{
	{
		ReadLockGuard g(m_mappings_lock);

		for( const auto &pair: m_mappings )
		{
			if( pair.second == atom )
			{
				return pair.first;
			}
		}
	}

	return cacheMiss(atom);
}

const std::string& XAtomMapper::cacheMiss(const XAtom &atom) const
{
	const auto name = XDisplay::getInstance().getName(atom);

	{
		WriteLockGuard g(m_mappings_lock);
		auto ret = m_mappings.insert( std::make_pair( name, atom ) );

		return (ret.first)->first;
	}
}

XAtom XAtomMapper::cacheMiss(const std::string &s)
{
	auto &logger = xwmfs::StdLogger::getInstance();
	XAtom ret = XAtom( XDisplay::getInstance().getAtom(s) );

	logger.debug() << "Resolved atom id for '"
		<< s << "' is " << std::dec << ret.get() << std::endl;

	{
		WriteLockGuard g(m_mappings_lock);
		m_mappings.insert( std::make_pair( s, (Atom)ret ) );
	}

	return ret;
}

} // end ns

std::ostream& operator<<(std::ostream &o, const xwmfs::XAtom &atom)
{
	auto &mapper = xwmfs::XAtomMapper::getInstance();

	o << atom.get() << " (" << mapper.getName(atom) << ")";

	return o;
}

