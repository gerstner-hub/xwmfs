// libxpp
#include <xpp/XWindow.hxx>

// xwmfs
#include "main/SelectionDirEntry.hxx"
#include "main/SelectionOwnerFile.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs {

SelectionOwnerFile::SelectionOwnerFile(const std::string &n, const SelectionDirEntry &parent) :
		FileEntry{n}, m_selection_dir{parent} {
	updateOwners();
}

Entry::Bytes SelectionOwnerFile::read(
		OpenContext *ctx, char *buf, size_t size, off_t offset) {
	{
		/*
		 * see getSelectionOwner() for an explanation of why this
		 * isn't event based.
		 *
		 * NOTE: Here another multi-threading bug lingers. For some
		 * reason the event handling thread blocks in XNextEvent()
		 * holding the event lock, although XPending() returned
		 * non-zero. This is probably some breakage in the low-level
		 * workings of libX11 again.
		 * The result is that if a user is trying to read from this
		 * file, the read may block until some unrelated X event
		 * wakes up the event thread, so our thread here can get the
		 * lock.
		 *
		 * Don't know what to do against this at the moment.
		 */
		auto &xwmfs = Xwmfs::Xwmfs::getInstance();
		cosmos::MutexGuard g{xwmfs.getEventLock()};
		updateOwners();
	}

	return FileEntry::read(ctx, buf, size, offset);
}

void SelectionOwnerFile::updateOwners() {
	this->str("");

	xpp::XWindow owner;

	for (const auto &selection: m_selection_dir.getSelectionTypes()) {
		owner = xpp::XWindow{m_selection_dir.getSelectionOwner(selection.first)};

		(*this) << selection.second << ": ";
		if (owner.id() == xpp::WinID::INVALID)
			(*this) << "0";
		else 
			(*this) << xpp::to_string(owner.id());
		*this << "\n";
	}
}

} // end ns
