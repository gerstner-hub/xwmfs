#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include "fuse/xwmfs_fuse_ops.h"
#include "fuse/xwmfs_fuse.hxx"
#include "main/Options.hxx"
#include "main/StdLogger.hxx"
#include "main/Xwmfs.hxx"
#include "common/Exception.hxx"

bool parseXWMFSOptions(int argc, char **argv, struct fuse_args &fuse_args)
{
	bool ret = false;
	// TODO: obtaining the XWMFS instance in this context is not so good.
	// It causes initialization of the XDisplay and such. If this fails
	// then our operation fails before we were even able to handle command
	// line parsing.
	xwmfs::Options &opts = xwmfs::Options::getInstance();

	for(int i = 0; i < argc; i++)
	{
		if( ::strcmp(argv[i], "--xsync") == 0 )
		{
			opts.xsync(true);
		}
		else if( ::strncmp(argv[i], "--logger=",
			sizeof("--logger=") - 1 ) == 0 )
		{
			std::string logger_opts = argv[i] +
				sizeof("--logger=") - 1;

			bool channels[4] = { true, true, true, false };

			for( size_t channel = 0;
				// if there are too few channels or too many
				// then the missing channels are set to
				// default or the superfluous data is ignored.
				//
				// if characters different then '0', '1' are
				// encountered then '1' is assumend.
				channel < logger_opts.length() &&
					channel < 4;
				channel ++ )
			{
				channels[channel] =
					(logger_opts[channel] == '0' ?
						false : true);
			}

			xwmfs::StdLogger::getInstance().setChannels(
				channels[0], channels[1],
				channels[2], channels[3]
			);
		}
		else
		{
			if( (::strcmp(argv[i], "--help") == 0) ||
				(::strcmp(argv[i], "-h") == 0) )
			{
				ret = true;
			}

			fuse_opt_add_arg(&fuse_args, argv[i]);
		}
	}

	return ret;
}

void printXWMFSHelp()
{
	fprintf(stderr, "\n\nxwmfs specific options:\n\n"
		"\t--xsync\n"
		"\t\toperate xlib calls synchronously for better error detection\n"
		"\t--logger=EWID\n"
		"\t\tset logger output for error (E), warning(W), info (I)\n"
		"\t\tand debug (D) to on ('1') or off ('0'), i.e. a row of four bits\n");
}
		
int main(int argc, char *argv[])
{
	try
	{
		auto &logger = xwmfs::StdLogger::getInstance();
		if( ! ::setlocale(LC_ALL, NULL) )
		{
			logger.error() << "Couldn't set locale\n";
		}

		// early initialization logic for X11 must be called before
		// any other X11 stuff happens
		xwmfs::Xwmfs::early_init();
		
		struct fuse_args fuse_args = FUSE_ARGS_INIT(0, NULL);

		
		bool print_xwmfs_help = false;
		
		try
		{
			print_xwmfs_help = parseXWMFSOptions(
				argc, argv, fuse_args
			);
		}
		catch( const xwmfs::Exception &e )
		{
			std::cerr << "Error initializing XWMFS:\n"
				<< e.what() << "\n";
			std::cerr << "Probably you're not running X or your "
				"window manager isn't supporting the EWMH "
				"protocol\n\n";
			return EXIT_FAILURE;
		}

		// the actual initialization is currently done via init and
		// destroy functions called from FUSE
		const int fuse_res = fuse_main(
			fuse_args.argc, fuse_args.argv,
			&xwmfs_oper, NULL
		);
					
		if( print_xwmfs_help )
		{
			printXWMFSHelp();
		}

		return fuse_res;
	}
	catch( const xwmfs::Exception &e )
	{
		std::cerr << "Caught exception in main: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}

