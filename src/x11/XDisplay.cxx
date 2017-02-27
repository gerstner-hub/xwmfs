#include "x11/XDisplay.hxx"

namespace xwmfs
{

XDisplay::~XDisplay()
{
	XCloseDisplay(m_dis);
	m_dis = nullptr;
}

XDisplay::XDisplay()
{
	// if nullptr is specified, then the value of DISPLAY
	// environment will be used
	m_dis = XOpenDisplay(nullptr);

	if( ! m_dis )
	{
		xwmfs_throw(DisplayOpenError());
	}
}

XDisplay& XDisplay::getInstance()
{
	static XDisplay dis;

	return dis;
}

XDisplay::AtomMappingError::AtomMappingError(
	Display *dis, const int errcode, const std::string &s
) :
	X11Exception(dis, errcode)	
{
	m_error += ". While trying to map " + \
		s + " to a valid atom.";
}

XDisplay::DisplayOpenError::DisplayOpenError() :
	Exception("Unable to open X11 display: \"")
{
	m_error += XDisplayName(nullptr);
	m_error += "\". ";
	m_error += "Is X running? Is the DISPLAY environment "\
		"variable correct?";
}

} // end ns

