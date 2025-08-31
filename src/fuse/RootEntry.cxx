#include <string_view>

// xwmfs
#include "fuse/RootEntry.hxx"

namespace xwmfs {

Entry* RootEntry::findEntry(const std::string_view path) {
	// should start with the root
	assert(path[0] == '/');

	// the current directory entry we're looking at - starting with
	// ourselves, of course
	xwmfs::DirEntry *cur_dir = this;
	// current path element
	std::string_view element;
	// start of the current path element string
	size_t start = 1;
	// end of the current path element string (at the path separator or end of string)
	size_t end = path.find_first_of('/', start);
	// whether this is the last path element in `path`
	bool is_final_element = (end == element.npos);

	xwmfs::Entry *ret = this;

	// if start equals the end then the last element has been reached
	while (start < path.size()) {
		// look at the current element.
		element = path.substr(start, end-start);

		if (!is_final_element) {
			cur_dir = xwmfs::Entry::tryCastDirEntry(cur_dir->getEntry(element));

			// either the element is no directory or not existing
			if (!cur_dir) {
				return nullptr;
			}
		} else {
			// the final element may also be a file instead of a directory
			ret = cur_dir->getEntry(element);
			// last element is not existing
			if (!ret) {
				return nullptr;
			}
		}

		if (is_final_element) {
			// terminating condition
			start = end;
		} else {
			// the next element starts one character after current
			// end (after the slash).
			start = end + 1;
			end = path.find_first_of('/', start);

			// the second part of the check is for cases of
			// "/some/path/" with a trailing slash
			if (end == element.npos || end + 1 == element.size()) {
				is_final_element = true;
				// this is for cases with an ending '/'
				if (start >= path.size()) {
					ret = cur_dir;
				}
			}
		}
	}

	// we should have found a return entry if we reach this part of the
	// code
	assert(ret);

	return ret;
}

} // end ns
