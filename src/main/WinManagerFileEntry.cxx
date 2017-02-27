// xwmfs
#include "main/WinManagerFileEntry.hxx"
#include "main/StdLogger.hxx"
#include "main/Xwmfs.hxx"
#include "x11/RootWin.hxx"
#include "x11/XWindow.hxx"

namespace xwmfs
{

const WinManagerFileEntry::SetIntFunctionMap WinManagerFileEntry::m_set_int_function_map =
{
	{ "active_desktop", &RootWin::setWM_ActiveDesktop },
	{ "number_of_desktops", &RootWin::setWM_NumDesktops }
};

const WinManagerFileEntry::SetWindowFunctionMap WinManagerFileEntry::m_set_window_function_map =
{
	{ "active_window", &RootWin::setWM_ActiveWindow }
};

int WinManagerFileEntry::write(const char *data, const size_t bytes, off_t offset)
{
	if( !m_writable )
		return -EBADF;
	// we don't support writing at offsets
	if( offset )
		return -EOPNOTSUPP;

	auto &root_win = xwmfs::Xwmfs::getInstance().getRootWin();
	auto &logger = xwmfs::StdLogger::getInstance();

	try
	{
		int the_num = 0;
		const auto parsed = parseInteger(data, bytes, the_num);

		if( parsed < 0 )
		{
			logger.warn() << ": Failed to parse integer for write to: "
				<< this->m_name << "\n";
			return parsed;
		}
	
		MutexGuard g(m_parent->getLock());

		auto int_it = m_set_int_function_map.find(m_name);

		if( int_it != m_set_int_function_map.end() )
		{
			auto set_func = int_it->second;
			((root_win).*set_func)(the_num);
			return bytes;
		}

		auto window_it = m_set_window_function_map.find(m_name);

		if( window_it != m_set_window_function_map.end() )
		{
			auto set_func = window_it->second;
			((root_win).*set_func)(XWindow(the_num));
			return bytes;
		}

		logger.warn()
			<< __FUNCTION__
			<< ": Write call for win manager file of unknown type: \""
			<< this->m_name
			<< "\"\n";
		return -ENXIO;
	}
	catch( const xwmfs::XWindow::NotImplemented &e )
	{
		return -ENOSYS;
	}
	catch( const xwmfs::Exception &e )
	{
		logger.error()
			<< __FUNCTION__ << ": Error setting window manager property ("
			<< this->m_name << "): " << e.what() << std::endl;
		return -EINVAL;
	}

	// claim we've written all content. This "eats up" things like
	// newlines passed with 'echo'.  Otherwise programs may try to
	// write the "missing" bytes which turns into an offset write
	// then, which we don't support
	return bytes;
}

} // end ns

