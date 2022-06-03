#include "transaction_context/transaction_context.h"
#include "transaction_context/global_context.h"

namespace scs
{

TransactionContext::TransactionContext(uint64_t gas_limit, uint64_t gas_rate_bid, Hash tx_hash)
	: current_priority(0, gas_rate_bid, tx_hash, 0)
	, invocation_stack()
	, runtime_stack()
	, gas_limit(gas_limit)
	, gas_rate_bid(gas_rate_bid)
	, gas_used(0)
	, return_buf()
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
