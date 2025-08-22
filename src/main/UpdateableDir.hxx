#pragma once

// C++
#include <map>
#include <vector>

// xwmfs
#include "fuse/DirEntry.hxx"
#include "x11/XAtom.hxx"

namespace xwmfs {

/**
 * \brief
 * 	Base class for directories that contain updateable files
 **/
template <typename CLASS>
class UpdateableDir :
		public DirEntry {
protected: // types

	using UpdateFunction = void (CLASS::*)(FileEntry &entry);
	using AtomVector = std::vector<XAtom>;

	//! holds information about a single file entry
	struct EntrySpec {
		//! the name of the entry
		const char *name = nullptr;
		//! whether this is a read-only or read-write entry
		const bool read_write = false;
		//! if set then the update function is called for any atom,
		//! not just the ones in \c atoms
		const bool always_update = false;
		//! a member function of the derived class type to call for
		//! updates of the entry value
		UpdateFunction member_func = nullptr;
		//! the associated XAtoms, if any
		AtomVector atoms;

		EntrySpec(const char *n, UpdateFunction f, const bool rw = false, const bool au = false) :
			name{n}, read_write{rw}, always_update{au}, member_func{f}, atoms{{}} {
		}

		EntrySpec(const char *n, UpdateFunction f, const bool rw, XAtom a) :
			name{n}, read_write{rw}, member_func{f}, atoms{{a}} {
		}

		EntrySpec(const char *n, UpdateFunction f, const bool rw, AtomVector av) :
			name(n), read_write(rw), member_func(f), atoms(av) {
		}
	};

	//! a mapping of XAtom values to the corresponding file entry specs
	using AtomSpecMap = std::map<XAtom, EntrySpec>;
	using SpecVector = std::vector<EntrySpec>;

protected: // functions

	UpdateableDir(const std::string &n, const SpecVector &vec);

	void updateModifyTime();

	AtomSpecMap getUpdateMap() const;
	SpecVector getAlwaysUpdateSpecs() const;

protected: // data

	const SpecVector m_specs;
	const SpecVector m_always_update_specs;
	const AtomSpecMap m_atom_update_map;
};

} // end ns
