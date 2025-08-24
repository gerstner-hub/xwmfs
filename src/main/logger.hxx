#pragma once

#include <cosmos/io/ILogger.hxx>

namespace cosmos {

class ILogger;

} // end ns

namespace xwmfs {

extern cosmos::ILogger *logger;

void set_logger(cosmos::ILogger &_logger);

}
