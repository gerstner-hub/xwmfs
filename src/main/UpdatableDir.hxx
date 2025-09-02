#pragma once

// C++
#include <map>
#include <vector>

// libxpp
#include <xpp/types.hxx>

// xwmfs
#include "common/types.hxx"
#include "fuse/DirEntry.hxx"

namespace xwmfs {

/// Base class for directories that contain updatable files.
template <typename CLASS>
class UpdatableDir :
		public DirEntry {
protected: // types

	using UpdateFunction = void (CLASS::*)(FileEntry &entry);
	using AlwaysUpdate = cosmos::NamedBool<struct always_update_t, false>;

	/// Holds information about a single file entry.
	struct EntrySpec {
		/// The file-system name of the entry.
		const char *name = nullptr;
		/// Whether this is a read-only or read-write entry.
		const Writable writable;
		/// Whether to update for all atom changes ignoring `atoms`.
		const AlwaysUpdate always_update;
		/// Function in derived class to call upon update.
		UpdateFunction member_func = nullptr;
		/// The associated AtomIDs, if any.
		xpp::AtomIDVector atoms;

		EntrySpec(const char *n, UpdateFunction f,
				const Writable wrt = Writable{false},
				const AlwaysUpdate always = AlwaysUpdate{false}) :
			name{n}, writable{wrt},
			always_update{always},
			member_func{f}, atoms{{}} {
		}

		EntrySpec(const char *n, UpdateFunction f,
				const xpp::AtomID atom,
				const Writable wrt = Writable{false}) :
			name{n}, writable{wrt},
			member_func{f}, atoms{{atom}} {
		}

		EntrySpec(const char *n, UpdateFunction f,
				const xpp::AtomIDVector &av,
				const Writable wrt = Writable{false}) :
			name(n), writable(wrt), member_func(f), atoms(av) {
		}
	};

	/// A mapping of AtomID values to their corresponding file entry specs.
	using AtomSpecMap = std::map<xpp::AtomID, EntrySpec>;
	using SpecVector = std::vector<EntrySpec>;

protected: // functions

	UpdatableDir(const std::string &n, const SpecVector &vec);

	void updateModifyTime();

	AtomSpecMap getUpdateMap() const;
	SpecVector getAlwaysUpdateSpecs() const;

protected: // data

	const SpecVector m_specs;
	const SpecVector m_always_update_specs;
	const AtomSpecMap m_atom_update_map;
};

} // end ns
