#include "transaction_context/transaction_context.h"
#include "transaction_context/global_context.h"

namespace scs
{

TransactionContext::TransactionContext(
	uint64_t gas_limit, 
	uint64_t gas_rate_bid, 
	Hash tx_hash,
	Address const& sender,
	GlobalContext const& global_context,
	SerialDeltaBatch&& local_delta_batch)
	: tx_hash(tx_hash)
	, invocation_stack()
	, runtime_stack()
	, sender(sender)
	, local_delta_batch(std::move(local_delta_batch))
	, gas_limit(gas_limit)
	, gas_rate_bid(gas_rate_bid)
	, gas_used(0)
	, return_buf()
	, logs()
	, storage_proxy(global_context.state_db)
	{}

/*
DeltaPriority 
TransactionContext::get_next_priority(uint32_t priority)
{
	current_priority.delta_id_number++;
	current_priority.custom_priority = priority;
	return current_priority;
} */

wasm_api::WasmRuntime*
TransactionContext::get_current_runtime()
{
	if (runtime_stack.size() == 0)
	{
		throw std::runtime_error("no active runtime during get_current_runtime() call");
	}
	return runtime_stack.back();
}

const MethodInvocation& 
TransactionContext::get_current_method_invocation() const
{
	return invocation_stack.back();
}

void 
TransactionContext::pop_invocation_stack()
{
	invocation_stack.pop_back();
	runtime_stack.pop_back();
}

void 
TransactionContext::push_invocation_stack(
	wasm_api::WasmRuntime* runtime, 
	MethodInvocation const& invocation)
{
	invocation_stack.push_back(invocation);
	runtime_stack.push_back(runtime);
}

AddressAndKey 
TransactionContext::get_storage_key(InvariantKey const& key) const
{
	static_assert(sizeof(Address) + sizeof(InvariantKey) == sizeof(AddressAndKey), "size mismatch");

	AddressAndKey out;
	auto const& cur_addr = get_current_method_invocation().addr;
	std::memcpy(out.data(), cur_addr.data(), sizeof(Address));

	std::memcpy(out.data() + sizeof(Address), key.data(), sizeof(InvariantKey));
	return out;
}


const Address&
TransactionContext::get_msg_sender() const
{
	if (invocation_stack.size() <= 1)
	{
		return sender;
	}
	return invocation_stack[invocation_stack.size() - 2].addr;
}

const Hash&
TransactionContext::get_src_tx_hash() const
{
	return tx_hash;
}

} /* scs */
