#include "main/logger.hxx"

namespace xwmfs {

cosmos::ILogger *logger = nullptr;

void set_logger(cosmos::ILogger &_logger) {
	logger = &_logger;
}

} // end ns
