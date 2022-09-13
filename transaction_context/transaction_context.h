#pragma once

#include <cstdint>
#include <vector>

#include "transaction_context/method_invocation.h"
#include "storage_proxy/storage_proxy.h"

#include <wasm_api/wasm_api.h>

#include "xdr/transaction.h"
#include "xdr/storage.h"

namespace scs {

class GlobalContext;
class ModifiedKeysList;

class TransactionContext {

	Hash tx_hash;

	std::vector<MethodInvocation> invocation_stack;
	std::vector<wasm_api::WasmRuntime*> runtime_stack;

	Address sender;

	bool committed_to_statedb = false;

	void assert_not_committed()
	{
		if (committed_to_statedb)
		{
			throw std::runtime_error("double push deltas");
		}
	}

public:

	const uint64_t gas_limit;
	const uint64_t gas_rate_bid;
	uint64_t gas_used;

	std::vector<uint8_t> return_buf;

	std::vector<std::vector<uint8_t>> logs;

	StorageProxy storage_proxy;

	TransactionContext(
		uint64_t gas_limit, 
		uint64_t gas_rate_bid, 
		Hash tx_hash, 
		Address const& sender, 
		GlobalContext& scs_data_structures,
		ModifiedKeysList& modified_keys_list);

	wasm_api::WasmRuntime*
	get_current_runtime();

	const MethodInvocation& 
	get_current_method_invocation() const;

	const Address&
	get_msg_sender() const;

	const Hash&
	get_src_tx_hash() const;

	AddressAndKey 
	get_storage_key(InvariantKey const& key) const;

	void pop_invocation_stack();
	void push_invocation_stack(wasm_api::WasmRuntime* runtime, MethodInvocation const& invocation);

	bool
	__attribute__((warn_unused_result))
	push_storage_deltas();
};

} /* namespace scs */
