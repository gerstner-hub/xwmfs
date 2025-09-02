// C++
#include <array>
#include <iostream>
#include <string_view>

// cosmos
#include <cosmos/cosmos.hxx>
#include <cosmos/io/StdLogger.hxx>
#include <cosmos/locale.hxx>

// xpp
#include <xpp/Xpp.hxx>

// xwmfs
#include "fuse/xwmfs_fuse_ops.h"
#include "main/Exception.hxx"
#include "main/logger.hxx"
#include "main/main.hxx"
#include "main/Options.hxx"
#include "main/Xwmfs.hxx"

namespace xwmfs {

void Main::parseLoggerSettings(const std::string_view bits) {
	std::array<bool, 4> channels;

	/*
	 * If there are too few channels or too many then the missing channels
	 * are set to default or the superfluous data is ignored.
	 *
	 * If characters different then '0', '1' are encountered then '1' is
	 * assumed.
	 */
	for (size_t channel = 0; channel < bits.length() && channel < channels.size(); channel++) {
		channels[channel] = bits[channel] == '0' ?  false : true;
	}

	xwmfs::logger->setChannels(
		channels[0], channels[1], channels[2], channels[3]
	);
}

bool Main::parseOptions(const cosmos::StringViewVector &args, struct fuse_args &fuse_args) {
	bool ret = false;
	auto &opts = xwmfs::Options::getInstance();

	for (const auto arg: args) {
		if (arg == "--xsync") {
			opts.setXsync(true);
		} else if (arg.starts_with("--logger=")) {
			auto logger_opts = arg.substr(arg.find_first_of('=') + 1);
			parseLoggerSettings(logger_opts);
		} else if (arg == "--handle-pseudo-windows") {
			opts.setHandlePseudoWindows(true);
		} else {
			if (arg == "-h" || arg == "--help") {
				ret = true;
			}

			fuse_opt_add_arg(&fuse_args, arg.data());
		}
	}

	return ret;
}

void Main::printHelp() {
	std::cerr << "\n\nxwmfs specific options:\n\n"
		"\t--xsync\n"
		"\t\toperate xlib calls synchronously for better error detection\n"
		"\t--logger=EWID\n"
		"\t\tset logger output for error (E), warning(W), info (I)\n"
		"\t\tand debug (D) to on ('1') or off ('0'), i.e. a row of four bits\n"
		"\t--handle-pseudo-windows\n"
		"\t\talso include hidden and helper windows like popup menus\n"
		"\t\tand window decorations"
		"\n";
}

cosmos::ExitStatus Main::main(const std::string_view argv0, const cosmos::StringViewVector &args) {
	auto logger = cosmos::StdLogger{};
	xwmfs::set_logger(logger);

	try {
		cosmos::locale::set_to_default(cosmos::locale::Category::ALL);
	} catch (const std::exception &ex) {
		logger.error() << "Couldn't set locale: " << ex.what() << "\n";
	}

	// early initialization logic for X11 must be called before
	// any other X11 stuff happens
	Xwmfs::early_init();

	struct fuse_args fuse_args = FUSE_ARGS_INIT(0, NULL);
	fuse_opt_add_arg(&fuse_args, argv0.data());

	bool print_xwmfs_help = false;

	try {
		print_xwmfs_help = parseOptions(args, fuse_args);
	} catch (const std::exception &e) {
		std::cerr << "Error initializing XWMFS:\n" << e.what() << "\n";
		std::cerr << "Probably you're not running X or your "
			"window manager isn't supporting the EWMH "
			"protocol\n\n";
		return cosmos::ExitStatus::FAILURE;
	}

	try {
		xpp::Init xpp_init;

		// the actual initialization is currently done via init and
		// destroy functions called from FUSE
		const int fuse_res = fuse_main(fuse_args.argc, fuse_args.argv, &xwmfs_oper, NULL);

		if (print_xwmfs_help) {
			printHelp();
		}

		return cosmos::ExitStatus{fuse_res};
	} catch (const xpp::XDisplay::DisplayOpenError &error) {
		std::cerr << "Failed to open X11 display: " << error.what() << "\n";
		std::cerr << "Is X running? Is the DISPLAY environment variable set correctly?\n";
		return cosmos::ExitStatus::FAILURE;
	}
}

} // end ns

int main(const int argc, const char **argv) {
	return cosmos::main<xwmfs::Main>(argc, argv);
}
