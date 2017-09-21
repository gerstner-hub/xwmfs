// xwmfs
#include "fuse/AbortHandler.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs
{

AbortHandler::AbortHandler(Condition &cond) :
	m_cond(cond)
{}

void AbortHandler::abort(pthread_t thread)
{
	{
		MutexGuard g(m_cond.getMutex());
		m_abort_set.insert( thread );
	}

	m_cond.broadcast();
}

bool AbortHandler::wasAborted()
{
	const auto self = pthread_self();
	auto it = m_abort_set.find( self );

	if( it == m_abort_set.end() )
	{
		return false;
	}

	m_abort_set.erase(it);

	return true;
}

bool AbortHandler::prepareBlockingCall(Entry *file)
{
	auto &xwmfs = xwmfs::Xwmfs::getInstance();
	return xwmfs.registerBlockingCall(file);
}

void AbortHandler::finishedBlockingCall()
{
	auto &xwmfs = xwmfs::Xwmfs::getInstance();
	return xwmfs.unregisterBlockingCall();
}

} // end ns

