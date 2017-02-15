#ifndef XWMFS_WINDOWDIR_HXX
#define XWMFS_WINDOWDIR_HXX

// xwmfs
#include "fuse/xwmfs_fuse.hxx"
#include "x11/XWindow.hxx"

namespace xwmfs
{

/**
 * \brief
 * 	A FileEntry that is associated with an XWindow object
 * \details
 * 	This type is used for all files found within windows directories in
 * 	the file system. Depending on the actual file called the right
 * 	operations are performed at the associated window.
 **/
struct WindowFileEntry :
	public FileEntry
{
	//! Creates a WindowFileEntry associated with \c win
	WindowFileEntry(
		const std::string &n,
		const XWindow& win,
		const time_t &t = 0,
		const bool writable = true
	) :
		FileEntry(n, writable, t),
		m_win(win)
	{ }

	/**
	 * \brief
	 * 	Implementation of write() that updates window properties
	 * 	according to the file that is being written
	 **/
	int write(const char *data, const size_t bytes, off_t offset) override;

	void writeName(const char *data, const size_t bytes)
	{
		std::string name(data, bytes);
		m_win.setName( name );
	}

	void writeDesktop(const char *data, const size_t bytes)
	{
		int the_num;
		const auto parsed = parseInteger(
			data, bytes, the_num
		);

		if( parsed >= 0 )
		{
			m_win.setDesktop( the_num );
		}
	}

	void writeCommand(const char *data, const size_t bytes);

	/**
	 * \brief
	 * 	Compares this file system entries against the given window
	 * \details
	 * 	If this file system entry is associated with \c w then \c true
	 * 	is returned. \c false otherwise.
	 **/
	bool operator==(const XWindow &w) const { return m_win == w; }
	bool operator!=(const XWindow &w) const { return !((*this) == w); }

	/**
	 * \brief
	 * 	Casts the object to its associated XWindow type
	 **/
	operator XWindow&() { return m_win; }

protected: // types

	typedef void (WindowFileEntry::*WriteMemberFunction)(const char*, const size_t);
	typedef std::map<std::string, WriteMemberFunction> WriteMemberFunctionMap;

protected: // data

	//! XWindow associated with this FileEntry
	XWindow m_win; // currently a flat copy of the window, it's not much data

	// a mapping of file system names to their associated write functions
	static const WriteMemberFunctionMap m_write_member_function_map;
};

} // end ns

#endif // inc. guard

