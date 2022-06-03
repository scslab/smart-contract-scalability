#pragma once

#include <cstdint>
#include <vector>

#include "xdr/types.h"
#include "xdr/transaction.h"

namespace scs {

struct MethodInvocation
{
	const Address addr;

	// Copying from eth design:
	// method names are 4 bytes (of valid ascii);
	// Here, public/private is denoted by method prefix
	// full method name is pub{XXXX}
	const uint32_t method_name;
	const std::vector<uint8_t> calldata;

	std::string 
	get_invocable_methodname() const;

	MethodInvocation(TransactionInvocation const& root_invocation);
	MethodInvocation(const Address& addr, uint32_t method, std::vector<uint8_t>&& calldata);
};

} /* scs */
