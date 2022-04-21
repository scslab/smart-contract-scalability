#include "transaction_context/transaction_context.h"

namespace scs
{

DeltaPriority 
TransactionContext::get_next_priority()
{
	current_priority.delta_id_number++;
	return current_priority;
}

WasmRuntime*
TransactionContext::get_current_runtime()
{
	return runtime_stack.back();
}

MethodInvocation& 
TransactionContext::get_current_method_invocation()
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
TransactionContext::push_invocation_stack(WasmRuntime* runtime, MethodInvocation const& invocation)
{
	invocation_stack.push_back(invocation);
	runtime_stack.push_back(runtime);
}

} /* scs */
