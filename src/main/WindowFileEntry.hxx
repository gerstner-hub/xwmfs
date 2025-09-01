#pragma once

// libxpp
#include <xpp/XWindow.hxx>

// xwmfs
#include "common/types.hxx"
#include "fuse/FileEntry.hxx"

namespace xwmfs {

/// A FileEntry that is associated with an XWindow object.
/**
 * This type is used for all files found within window directories in the
 * file system. Depending on the actual file called the right operations are
 * performed at the associated window.
 **/
class WindowFileEntry :
	public FileEntry {
public:
	/// Creates a WindowFileEntry associated with `win`.
	WindowFileEntry(const std::string &n, const xpp::XWindow &win,
			const cosmos::RealTime &t = cosmos::RealTime{},
			const Writable writable = Writable{true}) :
		FileEntry{n, t, writable},
		m_win{win} {
	}

	/// Updates window properties in the context of the concrete file type.
	Bytes write(OpenContext *ctx, const char *data,
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

	/// Compares this file system entry against the given window.
	/**
	 * If this file system entry is associated with `w` then `true` is
	 * returned. `false` otherwise.
	 **/
	bool operator==(const xpp::XWindow &w) const { return m_win == w; }
	bool operator!=(const xpp::XWindow &w) const { return !((*this) == w); }

	/// Casts the object to its associated XWindow type.
	operator xpp::XWindow&() { return m_win; }

protected: // data

	/// XWindow associated with this FileEntry
	xpp::XWindow m_win;
};

} // end ns
