#pragma once

#include <cstdint>
#include <vector>

#include "transaction_context/method_invocation.h"
#include "transaction_context/transaction_results.h"

#include "storage_proxy/storage_proxy.h"

#include "contract_db/contract_db_proxy.h"

#include <wasm_api/wasm_api.h>

#include "xdr/transaction.h"
#include "xdr/storage.h"

namespace scs {

class GlobalContext;
class BlockContext;

class TransactionContext
{
	std::vector<MethodInvocation> invocation_stack;
	std::vector<wasm_api::WasmRuntime*> runtime_stack;

	bool committed_to_statedb = false;

	void assert_not_committed()
	{
		if (committed_to_statedb)
		{
			throw std::runtime_error("double push deltas");
		}
	}

	const SignedTransaction& tx;
	const Hash& tx_hash;

	uint64_t gas_used;

	BlockContext const& block_context;

public:

	std::vector<uint8_t> return_buf;

	std::unique_ptr<TransactionResults> tx_results;

	StorageProxy storage_proxy;
	ContractDBProxy contract_db_proxy;

	TransactionContext(
		SignedTransaction const& tx,
		Hash const& tx_hash, 
		GlobalContext& scs_data_structures,
		BlockContext& block_context_);

	std::unique_ptr<TransactionResults> extract_results()
	{
		return std::unique_ptr<TransactionResults>(tx_results.release());
	}

	wasm_api::WasmRuntime*
	get_current_runtime();

	const MethodInvocation& 
	get_current_method_invocation() const;

	const Address&
	get_self_addr() const
	{
		return get_current_method_invocation().addr;
	}

	const Address&
	get_msg_sender() const;

	const Hash&
	get_src_tx_hash() const;

	const ContractDBProxy&
	get_contract_db_proxy() const
	{
		return contract_db_proxy;
	}

	uint64_t get_block_number() const;

	const Contract& get_deployable_contract(uint32_t index) const;
	uint32_t get_num_deployable_contracts() const;

	AddressAndKey 
	get_storage_key(InvariantKey const& key) const;

	void pop_invocation_stack();
	void push_invocation_stack(wasm_api::WasmRuntime* runtime, MethodInvocation const& invocation);

	bool
	__attribute__((warn_unused_result))
	push_storage_deltas();
};

} /* namespace scs */
