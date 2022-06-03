#include "transaction_context/transaction_context.h"
#include "transaction_context/global_context.h"

namespace scs
{

TransactionContext::TransactionContext(
	uint64_t gas_limit, 
	uint64_t gas_rate_bid, 
	Hash tx_hash, 
	GlobalContext const& global_context)
	: current_priority(0, gas_rate_bid, tx_hash, 0)
	, invocation_stack()
	, runtime_stack()
	, gas_limit(gas_limit)
	, gas_rate_bid(gas_rate_bid)
	, gas_used(0)
	, return_buf()
	, logs()
	, storage_proxy(global_context.state_db)
	{}

DeltaPriority 
TransactionContext::get_next_priority(uint64_t priority)
{
	current_priority.delta_id_number++;
	current_priority.custom_priority = priority;
	return current_priority;
}

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

} /* scs */
