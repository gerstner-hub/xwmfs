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
	bool xsync() const { return m_xsync; }

	//! sets the synchronized X mode to \c val
	void xsync(const bool val) { m_xsync = val; }

	/**
	 * \brief
	 * 	Returns whether pseudo windows should be handled by Xwmfs
	 * \details
	 * 	Pseudo windows are windows that are no direct childs of the
	 * 	root window, or have the override_redirect flag set in the
	 * 	create event.
	 *
	 * 	These windows can be popups handled within applications, for
	 * 	example, or they can be window decorations added by the window
	 * 	manager.
	 **/
	bool handlePseudoWindows() const { return m_handle_pseudo_windows; }

	//! sets the pseudo windows handling to \c val
	void handlePseudoWindows(const bool val) { m_handle_pseudo_windows = val; }

	//! Returns the singleton instance of the options object
	static Options& getInstance()
	{
		static Options opt;

		return opt;
	}

private: // functions
	//! private ctor. to implement singelton
	Options() { }

	// singleton pattern
	Options(const Options &) = delete;

private: // data

	bool m_xsync = false;
	bool m_handle_pseudo_windows = false;
};

} // end ns

#endif // inc. guard

