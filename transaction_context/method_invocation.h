#pragma once

#include <cstdint>
#include <vector>

#include "xdr/types.h"

namespace scs {

struct MethodInvocation
{
	Address addr;
	std::string method_name;
	std::vector<uint8_t> calldata;
};

} /* scs */
