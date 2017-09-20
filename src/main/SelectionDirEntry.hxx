#ifndef XWMFS_SELECTIONDIRENTRY_HXX
#define XWMFS_SELECTIONDIRENTRY_HXX

// C++
#include <vector>
#include <string>

// xwmfs
#include "x11/XAtom.hxx"
#include "fuse/DirEntry.hxx"

namespace xwmfs
{

class EventFile;
class SelectionOwnerFile;
class SelectionAccessFile;

/**
 * \brief
 * 	A DirEntry that holds files regarding the X11 selection buffer
 * 	handling
 * \details
 * 	There a three pre-defined selection buffers:
 *
 * 	- PRIMARY
 * 	- SECONDARY
 * 	- CLIPBOARD
 *
 * 	SECONDARY isn't really used any more. The X-Server only keeps track of
 * 	which window currently "owns" a selection. The actual selection is
 * 	then found in the window itself.
 *
 * 	The selections also can have different formats (like encodings, or
 * 	image format), and may have a length limitation. For large data the
 * 	selection needs to be transferred in a chunked way, which I won't
 * 	implement for the purposes of xwmfs for now. 'xsel' is a good example
 * 	of how this could be done.
 *
 * 	If xwmfs wants to own a selection then it also needs to keep a window
 * 	open for this purpose.
 *
 * 	We provide the following files here:
 *
 * 	- owners: will contain a line per selection buffer followed by the
 * 	current owner window ID
 * 	- primary: 
 * 		- on read will produce the current PRIMARY selection,
 * 		implemented via an EventFile, because the xwmfs process needs
 * 		to request the selection from the owning window first.
 * 		- on write the xwmfs process will claim the PRIMARY selection
 * 		and provide the data to other windows when asked for
 * 	- secondary: same as primary for SECONDARY
 * 	- clipboard: same as primary for CLIPBOARD
 * 	- events: will provide change notifications when selection's owners
 * 	change.
 *
 *	The following page gives a good overview of the workings of X
 *	selection buffers:
 *
 *	https://www.uninformativ.de/blog/postings/2017-04-02/0/POSTING-en.html
 **/
class SelectionDirEntry :
	public DirEntry
{
public: // types

	typedef std::vector< std::pair<XAtom, std::string> > SelectionTypeVector;
	typedef std::vector< SelectionAccessFile* > SelectionAccessFileVector;

public: // functions

	explicit SelectionDirEntry();

	const SelectionTypeVector& getSelectionTypes() const
	{
		return m_selection_types;
	}

	/**
	 * \brief
	 * 	Returns the window that is the current owner of the given
	 * 	selection type
	 * \return
	 * 	On error, invalid \c type or when there is no selection owner
	 * 	then None is returned.
	 **/
	Window getSelectionOwner(const Atom type) const;

protected: // functions

	/**
	 * \brief
	 * 	Collects the atoms for all covered selection types in
	 * 	m_selection_types
	 **/
	void collectSelectionTypes();

	/**
	 * \brief
	 * 	Creates a SelectionAccessFile child entry for each covered
	 * 	selection type
	 **/
	void createSelectionAccessFiles();

protected: // data

	SelectionOwnerFile *m_owners = nullptr;
	EventFile *m_events = nullptr;
	SelectionTypeVector m_selection_types;
	SelectionAccessFileVector m_selection_access_files;
};

} // end ns

#endif // inc. guard

