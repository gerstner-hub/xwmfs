// xwmfs
#include "fuse/FileEntry.hxx"
#include "x11/XDisplay.hxx"
#include "x11/XAtom.hxx"
#include "main/SelectionDirEntry.hxx"
#include "main/SelectionOwnerFile.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs
{

SelectionDirEntry::SelectionDirEntry() :
	DirEntry("selections", Xwmfs::getInstance().getCurrentTime())
{
	collectSelectionTypes();
	m_owners = new SelectionOwnerFile("owners", *this);
	addEntry(m_owners);
}

Window SelectionDirEntry::getSelectionOwner(const Atom type) const
{
	// it seems there is no general event mechanism available to keep
	// track of the selection owner when we're not currently involved in a
	// selection owner/get ourselves. this requires us to refresh the
	// selection owner content upon each read in m_owners.
	//
	// https://stackoverflow.com/questions/28578220/process-receiving-x11-selectionnotify-event-xev-doesnt-show-the-event-why-is#28595450
	auto res = XGetSelectionOwner( XDisplay::getInstance(), type );

	return res;
}

void SelectionDirEntry::collectSelectionTypes()
{
	// there are constants XA_PRIMARY, XA_SECONDARY and XA_CLIPBOARD but
	// the latter requires the display as parameter, also it seems to
	// belong to libXmu all quite complicated just for some atoms.
	//
	// we can also just resolve atoms ourselves using the play string
	// names.
	// NOTE: for some reason resolving the SECONDARY atom fails with
	// BadValue, so good riddance.
	const char *selections[] = { "PRIMARY", "CLIPBOARD" };
	auto &mapper = XAtomMapper::getInstance();

	for( const auto type: selections )
	{
		XAtom atom = mapper.getAtom(type);

		m_selection_types.push_back( { atom, type } );
	}
}

} // end ns

