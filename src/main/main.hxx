#pragma once

// cosmos
#include <cosmos/main.hxx>

struct fuse_args;

namespace xwmfs {

/// Main application class.
/**
 * This class contains the main entry point, initializes libfuse and runs
 * `fuse_main()`.
 **/
class Main :
	public cosmos::MainContainerArgs {
protected:
	cosmos::ExitStatus main(const std::string_view argv0, const cosmos::StringViewVector &args) override;

	/// Parses custom command line options and passes the rest to libfuse.
	/**
	 * \return Whether the usage/help should be printed.
	 **/
	bool parseOptions(const cosmos::StringViewVector &args, struct fuse_args &fuse_args);

	void parseLoggerSettings(const std::string_view bits);

	void printHelp();
};

} // end ns
