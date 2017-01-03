// own header
#include "x11/XWindow.hxx"
#include "x11/XAtom.hxx"

#include "main/Xwmfs.hxx"

namespace xwmfs
{

XWindow::XWindow(Window win) :
	m_win(win),
	m_event_mask(0),
	m_std_props(StandardProps::instance())
{
}
	
std::string XWindow::getName() const
{
	try
	{
		xwmfs::Property<xwmfs::utf8_string> utf8_name;

		this->getProperty(m_std_props.atom_ewmh_window_name, utf8_name);

		return utf8_name.get().data;
	}
	catch( ... )
	{ }

	/*
	 *	If EWMH name property is not present then try to fall back to
	 *	ICCCM WM_NAME property
	 *	(at least I think that is ICCCM). This will not be in UTF8 but
	 *	in XA_STRING format
	 */

	xwmfs::Property<const char*> name;

	this->getProperty(m_std_props.atom_icccm_window_name, name);

	return name.get();
}

pid_t XWindow::getPID() const
{
	xwmfs::Property<int> pid;

	this->getProperty(m_std_props.atom_ewmh_window_pid, pid);

	return pid.get();
}

int XWindow::getDesktop() const
{
	xwmfs::Property<int> desktop_nr;

	this->getProperty(m_std_props.atom_ewmh_window_desktop, desktop_nr);

	return desktop_nr.get();
}


void XWindow::setName(const std::string &name)
{
	// XXX should try ewmh_name first
	
	xwmfs::Property<const char*> name_prop( name.c_str() );

	this->setProperty( m_std_props.atom_icccm_window_name, name_prop );
}

void XWindow::setDesktop(const int num)
{
	/*
	 * simply setting the property does nothing. We need to send a request
	 * to the root window ...
	 *
	 * if the WM honors the request then it will set the property itself
	 * and we will get an update
	 */
	xwmfs::Xwmfs::getInstance().getRootWin().sendRequest(
		m_std_props.atom_ewmh_window_desktop,
		num,
		this
	);
}


} // end ns

