#pragma once

/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include <vector>

#include "transaction_context/method_invocation.h"
#include "transaction_context/transaction_results.h"
#include "transaction_context/global_context.h"

#include "storage_proxy/storage_proxy.h"
#include "storage_proxy/transaction_rewind.h"

#include "contract_db/contract_db_proxy.h"

#include "lficpp/lficpp.h"

#include "xdr/transaction.h"
#include "xdr/storage.h"

namespace scs {

template<typename StateDB_t>
struct StorageCommitment : public utils::NonMovableOrCopyable
{
	TransactionRewind rewind;
	StorageProxy<StateDB_t>& proxy;
	Hash const& src_tx_hash;

	StorageCommitment(StorageProxy<StateDB_t>& proxy, Hash const& src_tx_hash)
		: rewind()
		, proxy(proxy)
		, src_tx_hash(src_tx_hash)
		{}

	template<typename ModificationLog>
	void commit(ModificationLog& keys)
	{
		rewind.commit();
		proxy.log_modified_keys(keys, src_tx_hash);
	}
};

template<typename GlobalContext_t>
class TransactionContext
{
	std::vector<MethodInvocation> invocation_stack;
	std::vector<LFIProc*> runtime_stack;

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

	const uint64_t current_block;

public:

	using StateDB_t = decltype(GlobalContext_t().state_db);

	void consume_gas(uint64_t gas_to_consume);

	std::vector<uint8_t> return_buf;

	std::unique_ptr<TransactionResultsFrame> tx_results;

	StorageProxy<StateDB_t> storage_proxy;
	ContractDBProxy contract_db_proxy;

	TransactionContext(
		SignedTransaction const& tx,
		Hash const& tx_hash, 
		GlobalContext_t& scs_data_structures,
		uint64_t current_block,
		std::optional<NondeterministicResults> const& = std::nullopt);

	std::unique_ptr<TransactionResults> extract_results()
	{
		// TODO some copies to be removed here
		auto out = std::make_unique<TransactionResults>(tx_results -> get_results());
		tx_results.reset();
		return out;
	}

	LFIProc*
	get_current_runtime();

	LFIProc const*
	get_current_runtime() const
	{
		return const_cast<TransactionContext*>(this)->get_current_runtime();
	}

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

	Hash
	get_invoked_tx_hash() const;

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

	WitnessEntry const&
	get_witness(uint64_t wit_idx) const;

	void pop_invocation_stack();
	void push_invocation_stack(LFIProc* runtime, MethodInvocation const& invocation);

	std::unique_ptr<StorageCommitment<StateDB_t>>
	__attribute__((warn_unused_result))
	push_storage_deltas();
};

} /* namespace scs */
