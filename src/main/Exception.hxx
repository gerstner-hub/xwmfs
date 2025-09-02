#pragma once

// cosmos
#include <cosmos/error/CosmosError.hxx>

namespace xwmfs {

class Exception :
	public cosmos::CosmosError {
public:
	Exception(const std::string &text,
			const cosmos::SourceLocation &loc = cosmos::SourceLocation::current()) :
		cosmos::CosmosError{"xwmfs-error", text, loc} {
	}
};

}; // end ns
