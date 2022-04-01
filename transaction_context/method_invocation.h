#pragma once

#include <cstdint>
#include <vector>

#include "xdr/types.h"

namespace scs {

struct MethodInvocation
{
	Address addr;

	// Copying from eth design:
	// method names are 4 bytes (of valid ascii);
	// Here, public/private is denoted by method prefix
	// full method name is pub_{XXXX}
	uint32_t method_name;
	std::vector<uint8_t> calldata;

	std::string 
	get_invocable_methodname() const;
};

} /* scs */
