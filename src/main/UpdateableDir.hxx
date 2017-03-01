#ifndef XWMFS_UPDATEABLEDIR_HXX
#define XWMFS_UPDATEABLEDIR_HXX

// C++
#include <map>
#include <vector>

// xwmfs
#include "fuse/DirEntry.hxx"

namespace xwmfs
{

/**
 * \brief
 * 	Base class for directories that contain updateable files
 **/
template <typename CLASS>
class UpdateableDir :
	public DirEntry
{
protected: // types

	typedef void (CLASS::*UpdateFunction)(FileEntry &entry);
	typedef std::vector<XAtom> AtomVector;

	//! holds information about a single file entry
	struct EntrySpec
	{
		//! the name of the entry
		const char *name = nullptr;
		//! whether this is a read-only or read-write entry
		const bool read_write = false;
		//! a member function of the derived class type to call for
		//! updates of the entry value
		UpdateFunction member_func = nullptr;
		//! the associated XAtoms, if any
		AtomVector atoms;

		EntrySpec(
			const char *n,
			UpdateFunction f,
			const bool rw = false
		) : name(n), read_write(rw), member_func(f), atoms({}) {}

		EntrySpec(
			const char *n,
			UpdateFunction f,
			const bool rw,
			XAtom a
		) : name(n), read_write(rw), member_func(f), atoms({a}) {}

		EntrySpec(
			const char *n,
			UpdateFunction f,
			const bool rw,
			AtomVector av
		) : name(n), read_write(rw), member_func(f), atoms(av) {}
	};

	//! a mapping of XAtom values to the corresponding file entry specs
	typedef std::map<XAtom, EntrySpec> AtomSpecMap;
	typedef std::vector<EntrySpec> SpecVector;

protected: // functions

	UpdateableDir(const std::string &n, const SpecVector &vec);

	void updateModifyTime();

	AtomSpecMap getUpdateMap() const;

protected: // data

	const SpecVector m_specs;
	const AtomSpecMap m_atom_update_map;
};

} // end ns

#endif // inc. guard

