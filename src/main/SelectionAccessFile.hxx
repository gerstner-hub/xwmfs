#pragma once

// cosmos
#include <cosmos/thread/Condition.hxx>

// libxpp
#include <xpp/types.hxx>
#include <xpp/XWindow.hxx>

// xwmfs
#include "fuse/FileEntry.hxx"

namespace xwmfs {

class SelectionDirEntry;

/// This file provides access to an arbitrary X selection buffer.
/**
 * This file type makes it possible to
 * 
 * - upon write, take ownership of a selection buffer and provide the written
 *   contents to other X clients
 * - upon read, request the selection buffer content from the current
 *   selection owner and return the data to the user
 * 
 * for any available type of X selection buffer.
 **/
class SelectionAccessFile :
		public FileEntry {
public: // functions

	/// Creates a new selection access file of the given name and type.
	/**
	 * \param[in] parent
	 * 	The parent directory which needs to be of type SelectionDirEntry.
	 * \param[in] type
	 * 	The atom identifier of the selection type this file entry
	 * 	should handle.
	 **/
	SelectionAccessFile(const std::string &n, SelectionDirEntry &parent, const xpp::AtomID type);

	Bytes read(OpenContext *ctx, char *buf, size_t size, off_t offset) override;

	Bytes write(OpenContext *ctx, const char *data, const size_t bytes, off_t offset) override;

	void reportConversionResult(const xpp::AtomID result_prop);

	void provideConversion(xpp::XWindow &requestor, const xpp::AtomID target_prop) const;

	/// \see EventFile::enableDirectIO()
	bool enableDirectIO() const override { return true; }

	xpp::AtomID type() const { return m_sel_type; }

protected: // functions

	/// Updates the cached owner information in m_owner.
	void updateOwner();

	/// Requests the current selection buffer contents and stores it as the file entry's data.
	/**
	 * throws cosmos::Errno on error.
	 **/
	void updateSelection();

protected: // data

	/// Parent dir with helper routines.
	SelectionDirEntry &m_parent;
	/// The X selection type we represent.
	const xpp::AtomID m_sel_type;
	/// The property where requested selection buffer conversions go to.
	const xpp::AtomID m_target_prop;
	/// Caches the current owner window of the selection we're representing.
	xpp::XWindow m_owner;
	cosmos::Condition m_result_cond;
	bool m_result_arrived = false;
	xpp::AtomID m_result_prop;
};

} // end ns
