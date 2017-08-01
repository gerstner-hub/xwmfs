// xwmfs
#include "main/Xwmfs.hxx"
#include "main/UpdateableDir.hxx"
#include "main/WinManagerDirEntry.hxx"
#include "main/WindowDirEntry.hxx"

namespace xwmfs
{

template <typename CLASS>
UpdateableDir<CLASS>::UpdateableDir(const std::string &n, const SpecVector &vec) :
	DirEntry(n, Xwmfs::getInstance().getCurrentTime()),
	m_specs(vec),
	m_always_update_specs(getAlwaysUpdateSpecs()),
	m_atom_update_map(getUpdateMap())
{
}

template <typename CLASS>
typename UpdateableDir<CLASS>::AtomSpecMap UpdateableDir<CLASS>::getUpdateMap() const
{
	AtomSpecMap ret;

	for( const auto &spec: m_specs )
	{
		for( const auto &atom: spec.atoms )
		{
			ret.insert( { atom, spec } );
		}
	}

	return ret;
}

template <typename CLASS>
typename UpdateableDir<CLASS>::SpecVector UpdateableDir<CLASS>::getAlwaysUpdateSpecs() const
{
	SpecVector ret;

	for( const auto &spec: m_specs )
	{
		if( spec.always_update )
		{
			ret.push_back(spec);
		}
	}

	return ret;
}

template <typename CLASS>
void UpdateableDir<CLASS>::updateModifyTime()
{
	m_modify_time = Xwmfs::getInstance().getCurrentTime();
}

/* explicit template instantiations */
template class UpdateableDir<WinManagerDirEntry>;
template class UpdateableDir<WindowDirEntry>;

} // end ns

