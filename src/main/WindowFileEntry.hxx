#pragma once

// libxpp
#include <xpp/XWindow.hxx>

// xwmfs
#include "fuse/FileEntry.hxx"

namespace xwmfs {

/**
 * \brief
 * 	A FileEntry that is associated with an XWindow object
 * \details
 * 	This type is used for all files found within windows directories in
 * 	the file system. Depending on the actual file called the right
 * 	operations are performed at the associated window.
 **/
class WindowFileEntry :
	public FileEntry {
public:
	//! Creates a WindowFileEntry associated with \c win
	WindowFileEntry(const std::string &n, const xpp::XWindow& win,
			const time_t &t = 0, const bool writable = true) :
		FileEntry{n, writable, t},
		m_win{win} {
	}

	/**
	 * \brief
	 * 	Implementation of write() that updates window properties
	 * 	according to the file that is being written
	 **/
	int write(OpenContext *ctx, const char *data,
			const size_t bytes, off_t offset) override;

	void writeName(const char *data, const size_t bytes) {
		std::string name(data, bytes);
		m_win.setName(name);
	}

	void writeDesktop(const char *data, const size_t bytes);

	void writeCommand(const char *data, const size_t bytes);

	void writeGeometry(const char *data, const size_t bytes);

	void writeProperties(const char *data, const size_t bytes);

	void setProperty(const std::string &name);

	void delProperty(const std::string &name);

	/**
	 * \brief
	 * 	Compares this file system entries against the given window
	 * \details
	 * 	If this file system entry is associated with \c w then \c true
	 * 	is returned. \c false otherwise.
	 **/
	bool operator==(const xpp::XWindow &w) const { return m_win == w; }
	bool operator!=(const xpp::XWindow &w) const { return !((*this) == w); }

	/**
	 * \brief
	 * 	Casts the object to its associated XWindow type
	 **/
	operator xpp::XWindow&() { return m_win; }

protected: // types

	using WriteMemberFunction = void (WindowFileEntry::*)(const char*, const size_t);
	using WriteMemberFunctionMap = std::map<std::string, WriteMemberFunction>;

protected: // data

	//! XWindow associated with this FileEntry
	xpp::XWindow m_win; // currently a flat copy of the window, it's not much data

	// a mapping of file system names to their associated write functions
	static const WriteMemberFunctionMap m_write_member_function_map;
};

} // end ns
