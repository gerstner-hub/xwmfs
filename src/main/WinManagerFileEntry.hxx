#ifndef XWMFS_WINMANAGERFILEENTRY_HXX
#define XWMFS_WINMANAGERFILEENTRY_HXX

// xwmfs
#include "fuse/xwmfs_fuse.hxx"

namespace xwmfs
{

class RootWin;
class XWindow;

/**
 * \brief
 * 	A FileEntry that is associated with a global window manager entry
 * \details
 * 	This is a specialized FileEntry for particular global entries relating
 * 	to the window manager. Mostly this is only used for writable files to
 * 	relay the write request correctly.
 **/
struct WinManagerFileEntry : 
	public FileEntry
{
	WinManagerFileEntry(const std::string &n, const time_t &t = 0) :
		FileEntry(n, true, t)
	{}

	int write(const char *data, const size_t bytes, off_t offset) override;

protected: // types

	typedef void (RootWin::*SetIntFunction)(const int&);
	typedef std::map<std::string, SetIntFunction> SetIntFunctionMap;
	typedef void (RootWin::*SetWindowFunction)(const XWindow&);
	typedef std::map<std::string, SetWindowFunction> SetWindowFunctionMap;
	
	// a mapping of file system names to their associated set int functions
	static const SetIntFunctionMap m_set_int_function_map;
	static const SetWindowFunctionMap m_set_window_function_map;
};

} // end ns

#endif // inc. guard

