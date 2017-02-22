#ifndef XWMFS_EVENT_FILE_HXX
#define XWMFS_EVENT_FILE_HXX

namespace xwmfs
{

/**
 * \brief
 * 	A special file that allows readers to block until new events arrive
 * \details
 * 	While all other file system entries contain some small defined amount
 * 	of data, an event file offers a potentielly endless stream of data as
 * 	new events are coming in.
 *
 * 	Events can be arbitrary strings to be delivered to readers. Multiple
 * 	readers may block on an EventFile until new data arrives.
 **/
class EventFile
{

};

} // end ns

#endif // inc. guard

