// C++
#include <exception>
#include <iostream>
#include <stdexcept>

// xwmfs
#include "main/Exception.hxx"

namespace {

class TerminateHandler;
// we need an extra pointer here to use the global instance in the class
// implementation
TerminateHandler *instance = nullptr;

/// This class sets up a global C++ exception and terminate handler.
class TerminateHandler {
public:
	TerminateHandler() {
		if (instance) {
			// LOL!
			std::terminate();
		}

		instance = this;

		m_orig_terminate = std::set_terminate(
			TerminateHandler::xwmfs_terminate
		);
	}

	void terminate() {
		if (isExceptionActive()) {
			std::cerr << "Uncaught exception occured!" << std::endl;
			printException();
		} else {
			std::cerr
				<< "Program terminated for an unknown reason (e.g. a pure virtual method call)"
				<< std::endl;
		}

		m_orig_terminate();
	}

	void printException() {
		try {
			// let's inspect the active exception then
			throw;
		} catch (const xwmfs::Exception &ex) {
			std::cerr << "xwmfs::Exception: " << ex.what() << "\n";
		} catch (const std::exception &ex) {
			std::cerr << "std::exception: " << ex.what() << "\n";
		} catch(...) {
			std::cerr << "Unknown exception type\n";
		}
	}

	bool isExceptionActive() {
		// NOTE: std::uncaught_exception doesn't work here, because it
		// returns false already if a terminate handler has been found
		// ... and guess what ... we're a terminate handler!
		return static_cast<bool>(std::current_exception());;
	}

	static void xwmfs_terminate(void) {
		instance->terminate();
	}

protected: // data

	std::terminate_handler m_orig_terminate;
};

TerminateHandler terminate_handler;

} // end ns
