#pragma once

#include <cstdint>

#include "transaction_context/method_invocation.h"

#include "wasm_api/wasm_api.h"

#include "xdr/transaction.h"

namespace scs {

struct TransactionContext {
	const uint64_t gas_limit;
	uint64_t gas_used;

	const Address source_account;

	std::vector<MethodInvocation> invocation_stack;
	std::vector<uint8_t> return_buf;
	std::vector<WasmRuntime*> runtime_stack;

	std::vector<std::vector<uint8_t>> logs;

	TransactionContext(uint64_t gas_limit, Address const& src)
		: gas_limit(gas_limit)
		, gas_used(0)
		, source_account(src)
		, invocation_stack()
		, return_buf()
		, runtime_stack()
		{}

};

} /* namespace scs */
