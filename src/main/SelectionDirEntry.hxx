#pragma once

// C++
#include <string>
#include <vector>

// libxpp
#include <xpp/fwd.hxx>
#include <xpp/types.hxx>

// xwmfs
#include "fuse/DirEntry.hxx"

namespace xwmfs {

class EventFile;
class SelectionOwnerFile;
class SelectionAccessFile;

/// A DirEntry that holds files regarding the X11 selection buffer handling
/**
 * There a three pre-defined selection buffers:
 *
 * - PRIMARY
 * - SECONDARY
 * - CLIPBOARD
 *
 * SECONDARY isn't really used any more. The X-Server only keeps track of
 * which window currently "owns" a selection. The actual selection is
 * then found in the window itself.
 *
 * The selections can also have different formats (like encodings, or
 * image format), and may have a length limitation. For large data the
 * selection needs to be transferred in a chunked way, which I won't
 * implement for the purposes of xwmfs for now. 'xsel' is a good example
 * of how this could be done.
 *
 * If xwmfs wants to own a selection then it also needs to keep a window
 * open for this purpose.
 *
 * We provide the following files here:
 *
 * - owners: will contain a line per selection buffer followed by the
 *   current owner's window ID
 * - primary:
 * 	- on read will produce the current PRIMARY selection,
 * 	implemented via an EventFile, because the xwmfs process needs
 * 	to request the selection from the owning window first.
 * 	- on write the xwmfs process will claim the PRIMARY selection
 * 	and provide the data to other windows when asked for
 * - secondary: same as primary for SECONDARY
 * - clipboard: same as primary for CLIPBOARD
 * - events: will provide change notifications when selections' owners
 *   change.
 *
 * The following page gives a good overview of the workings of X
 * selection buffers:
 *
 * https://www.uninformativ.de/blog/postings/2017-04-02/0/POSTING-en.html
 **/
class SelectionDirEntry :
		public DirEntry {
public: // types

	using SelectionTypeVector = std::vector<std::pair<xpp::AtomID, std::string>>;
	using SelectionAccessFileVector = std::vector<SelectionAccessFile*>;

public: // functions

	explicit SelectionDirEntry();

	const SelectionTypeVector& getSelectionTypes() const {
		return m_selection_types;
	}

	/// Returns the window that is the current owner of the given selection type.
	/**
	 * On error, invalid `type` or when there is no selection owner then
	 * None is returned.
	 **/
	xpp::WinID getSelectionOwner(const xpp::AtomID type) const;

	/// A selection buffer request has been answered.
	void conversionResult(const xpp::SelectionEvent &ev);

	/// Somebody requested a selection we own.
	void conversionRequest(const xpp::SelectionRequestEvent &ev);

	/// Ownership of a selection was lost.
	void lostOwnership(const xpp::SelectionClearEvent &ev);

protected: // functions

	/// Collects the atoms for all covered selection types in m_selection_types.
	void collectSelectionTypes();

	// Creates a SelectionAccessFile child entry for each covered selection type.
	void createSelectionAccessFiles();

	void replyConversionRequest(const xpp::SelectionRequestEvent &ev, const bool good);

	/// Returns the label for the selection buffer identified by `atom`.
	std::string selectionBufferLabel(const xpp::AtomID atom) const;

protected: // data

	SelectionOwnerFile *m_owners = nullptr;
	EventFile *m_events = nullptr;
	SelectionTypeVector m_selection_types;
	SelectionAccessFileVector m_selection_access_files;
};

} // end ns
