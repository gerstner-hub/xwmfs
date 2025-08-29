#pragma once

#include <cosmos/io/ILogger.hxx>

namespace xwmfs {

extern cosmos::ILogger *logger;

void set_logger(cosmos::ILogger &_logger);

} // end ns
