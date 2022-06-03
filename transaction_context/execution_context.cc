#include "transaction_context/execution_context.h"
#include "transaction_context/global_context.h"

#include "crypto/hash.h"

#include "debug/debug_macros.h"

#include "builtin_fns/builtin_fns.h"

namespace scs
{

void 
ExecutionContext::invoke_subroutine(MethodInvocation const& invocation)
{
	auto iter = active_runtimes.find(invocation.addr);
	if (iter == active_runtimes.end())
	{
		CONTRACT_INFO("creating new runtime for contract at %s", debug::array_to_str(invocation.addr).c_str());
		active_runtimes.emplace(invocation.addr, wasm_context.new_runtime_instance(invocation.addr));
		BuiltinFns::link_fns(*active_runtimes.at(invocation.addr));
	}

	auto* runtime = active_runtimes.at(invocation.addr).get();

	tx_context -> push_invocation_stack(runtime, invocation);

	runtime->invoke(invocation.get_invocable_methodname().c_str());

	tx_context -> pop_invocation_stack();
}

TransactionStatus
ExecutionContext::execute(Transaction const& root)
{
	if (executed)
	{
		throw std::runtime_error("can't reuse execution context!");
	}
	executed = true;

	MethodInvocation invocation(root.invocation);

	tx_context = std::make_unique<TransactionContext>(root.gas_limit, root.gas_rate_bid, hash_xdr(root), scs_data_structures);

	try
	{
		invoke_subroutine(invocation);
	} 
	catch(wasm_api::WasmError& e)
	{
		std::printf("Execution error: %s\n", e.what());
		return TransactionStatus::FAILURE;
	}
	catch(...)
	{
		std::printf("unrecoverable error!\n");
		std::abort();
	}

	return TransactionStatus::SUCCESS;
}

void
ExecutionContext::reset()
{
	// nothing to do to clear wasm_context
	active_runtimes.clear();
	tx_context.reset();
	executed = false;
}

TransactionContext& 
ExecutionContext::get_transaction_context()
{
	if (!executed)
	{
		throw std::runtime_error("can't get ctx before execution");
	}
	return *tx_context;
}

std::vector<std::vector<uint8_t>> const&
ExecutionContext::get_logs()
{
	if (!executed) 
	{
		throw std::runtime_error("can't get logs before execution");
	}
	return tx_context -> logs;
}

} /* scs */
