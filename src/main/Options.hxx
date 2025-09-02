#pragma once

namespace xwmfs {

/// Simple class to store global XWMFS program options
/**
 * These options are set in the main() during parsing command line arguments.
 *
 * This is a singleton class, mostly because of initialization order issues
 * with the Xwmfs singleton. We could place this as a regular member into
 * Xwmfs, but then the command line parsing would already constructor the
 * Xwmfs instance, possibly aborting due to X11 access issues.
 **/
class Options {
public: // functions

	/// Returns whether the synchronized mode is set in the options.
	bool xsync() const { return m_xsync; }

	/// Sets the synchronized X mode to \c val
	void setXsync(const bool val) { m_xsync = val; }

	/// Returns whether pseudo windows should be handled by Xwmfs.
	/**
	 * Pseudo windows are windows that are no direct children of the root
	 * window, or have the override_redirect flag set in the create event.
	 *
	 * These windows can be popups handled within applications, for
	 * example, or they can be window decorations added by the window
	 * manager.
	 **/
	bool handlePseudoWindows() const { return m_handle_pseudo_windows; }

	/// Sets the pseudo windows handling to \c val
	void setHandlePseudoWindows(const bool val) { m_handle_pseudo_windows = val; }

	/// Returns the singleton instance of the options object
	static Options& getInstance() {
		static Options opt;

		return opt;
	}

private: // functions

	/// Private constructor to implement singleton pattern.
	Options() {}

	/// singleton pattern
	Options(const Options &) = delete;

private: // data

	bool m_xsync = false;
	bool m_handle_pseudo_windows = false;
};

} // end ns
