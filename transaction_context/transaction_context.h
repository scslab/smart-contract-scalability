#pragma once

#include <cstdint>
#include <vector>

#include "transaction_context/method_invocation.h"
#include "transaction_context/storage_cache.h"

#include <wasm_api/wasm_api.h>

#include "xdr/transaction.h"
#include "xdr/storage.h"

namespace scs {

class GlobalContext;

class TransactionContext {

	DeltaPriority current_priority;

	std::vector<MethodInvocation> invocation_stack;
	std::vector<wasm_api::WasmRuntime*> runtime_stack;

	Address sender;

public:

	const uint64_t gas_limit;
	const uint64_t gas_rate_bid;
	uint64_t gas_used;

	std::vector<uint8_t> return_buf;

	std::vector<std::vector<uint8_t>> logs;

	StorageCache storage_proxy;

	TransactionContext(uint64_t gas_limit, uint64_t gas_rate_bid, Hash tx_hash, Address const& sender, GlobalContext const& scs_data_structures);

	DeltaPriority 
	get_next_priority(uint64_t priority);

	wasm_api::WasmRuntime*
	get_current_runtime();

	const MethodInvocation& 
	get_current_method_invocation() const;

	const Address&
	get_msg_sender() const;

	void pop_invocation_stack();
	void push_invocation_stack(wasm_api::WasmRuntime* runtime, MethodInvocation const& invocation);

};

} /* namespace scs */
