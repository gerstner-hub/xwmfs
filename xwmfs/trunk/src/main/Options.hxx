#ifndef XWMFS_OPTIONS_HXX
#define XWMFS_OPTIONS_HXX

namespace xwmfs
{

/**
 * \brief
 * 	Simple class to store global XWMFS program options
 * \details
 * 	These options are set in the main() during parsing command line
 * 	arguments.
 *
 * 	It is a singleton class.
 **/
class Options
{
public: // functions

	//! returns whether the synchronized mode is set in the options
	bool xsync() const
	{ return m_xsync; }

	//! sets the synchronized X mode to \c val
	void xsync(const bool val)
	{ m_xsync = val; }

	//! Returns the singleton instance of the options object
	static Options& getInstance()
	{
		static Options opt;

		return opt;
	}

private: // functions
	//! private ctor. to implement singelton
	Options() :
		m_xsync(false)
	{ }

	// singleton pattern
	Options(const Options &) = delete;

private: // data
	bool m_xsync;
};

} // end ns

#endif // inc. guard

