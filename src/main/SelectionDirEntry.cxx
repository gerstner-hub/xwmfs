// cosmos
#include <cosmos/string.hxx>

// libxpp
#include <xpp/AtomMapper.hxx>
#include <xpp/event/SelectionClearEvent.hxx>
#include <xpp/event/SelectionEvent.hxx>
#include <xpp/event/SelectionRequestEvent.hxx>

// xwmfs
#include "fuse/FileEntry.hxx"
#include "main/SelectionAccessFile.hxx"
#include "main/SelectionDirEntry.hxx"
#include "main/SelectionOwnerFile.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs {

SelectionDirEntry::SelectionDirEntry() :
		DirEntry{"selections", Xwmfs::getInstance().getCurrentTime()} {
	collectSelectionTypes();
	m_owners = new SelectionOwnerFile{"owners", *this};
	addEntry(m_owners);
	createSelectionAccessFiles();
}

xpp::WinID SelectionDirEntry::getSelectionOwner(const xpp::AtomID _type) const {
	// it seems there is no general event mechanism available to keep
	// track of the selection owner when we're not currently involved in a
	// selection owner/get ourselves. this requires us to refresh the
	// selection owner content upon each read in m_owners.
	//
	// https://stackoverflow.com/questions/28578220/process-receiving-x11-selectionnotify-event-xev-doesnt-show-the-event-why-is#28595450
	auto &display = Xwmfs::getInstance().getDisplay();
	auto winid = display.selectionOwner(_type);
	return winid ? *winid : xpp::WinID::INVALID;
}

void SelectionDirEntry::collectSelectionTypes() {
	// there are constants XA_PRIMARY, XA_SECONDARY and XA_CLIPBOARD but
	// the latter requires the display as parameter, also it seems to
	// belong to libXmu all quite complicated just for some atoms.
	//
	// we can also just resolve atoms ourselves using the play string
	// names.
	// NOTE: for some reason resolving the SECONDARY atom fails with
	// BadValue, so good riddance.
	const char *selections[] = {"PRIMARY", "CLIPBOARD"};

	for (const auto _type: selections) {
		const auto atom = xpp::atom_mapper.mapAtom(_type);
		m_selection_types.push_back({atom, _type});
	}
}

void SelectionDirEntry::createSelectionAccessFiles() {
	SelectionAccessFile *file;

	for (const auto &info: m_selection_types) {
		file = new SelectionAccessFile{
			cosmos::to_lower(info.second),
			*this,
			info.first
		};

		m_selection_access_files.push_back(file);
		addEntry(file);
	}
}


std::string SelectionDirEntry::selectionBufferLabel(const xpp::AtomID atom) const {
	for (const auto &selection: m_selection_types) {
		if (selection.first == atom) {
			return selection.second;
		}
	}

	return "unknown";
}

void SelectionDirEntry::conversionResult(const xpp::SelectionEvent &ev) {
	logger->info() << "Got conversion result for selection buffer '"
		<< selectionBufferLabel(ev.selection()) << "'\n";

	for (auto &file: m_selection_access_files) {
		if (file->type() == ev.selection()) {
			file->reportConversionResult(ev.property());
			break;
		}
	}
}

void SelectionDirEntry::conversionRequest(const xpp::SelectionRequestEvent &ev) {
	SelectionAccessFile *selection_file = nullptr;

	for (auto &file: m_selection_access_files) {
		if (file->type() == ev.selection()) {
			selection_file = file;
		}
	}

	if (ev.target() != xpp::atoms::ewmh_utf8_string || !selection_file) {
		/* either an unsupported conversion target was requested or an
		 * unknown selection buffer addressed */
		replyConversionRequest(ev, false);
		return;
	}

	// the file will cause the target property to be written correctly
	xpp::XWindow requestor{ev.requestor()};
	selection_file->provideConversion(requestor, ev.property());
	replyConversionRequest(ev, true);
}

void SelectionDirEntry::replyConversionRequest(
		const xpp::SelectionRequestEvent &req, const bool good) {
	logger->error() << "Failed to convert selection buffer '"
		<< selectionBufferLabel(req.selection())
		<< "' to requested target format "
		<< cosmos::to_integral(req.target()) << "\n";

	xpp::Event reply{xpp::EventType::SELECTION_NOTIFY};
	xpp::SelectionEventBuilder selection{reply};
	selection.setRequestor(req.requestor());
	selection.setSelection(req.selection());
	selection.setTarget(req.target());
	// None indicates that we can't comply to the request
	selection.setProperty(good ? req.property() : xpp::AtomID::INVALID);
	selection.setTime(req.time());

	xpp::XWindow requestor{req.requestor()};
	requestor.sendEvent(reply);
}

void SelectionDirEntry::lostOwnership(const xpp::SelectionClearEvent &ev) {
	// don't know if we should do anything here like clearing the
	// selection data?
	logger->info() << "Lost ownership of selection buffer '"
		<< selectionBufferLabel(ev.selection()) << "'\n";
}

} // end ns
