#ifndef XWMFS_ITHREADENTRY_HXX
#define XWMFS_ITHREADENTRY_HXX

namespace xwmfs
{

class Thread;

/**
 * \brief
 *	Interface used for threads to run in
 **/
class IThreadEntry
{
public:
	//! entry function for a thread
	virtual void threadEntry(const Thread &t) = 0;
};

} // end ns

#endif // inc. guard

