#include "transaction_context/execution_context.h"
#include "transaction_context/global_context.h"
#include "transaction_context/threadlocal_context.h"

#include "tx_block/tx_block.h"

#include "crypto/hash.h"

#include "debug/debug_macros.h"

#include "builtin_fns/builtin_fns.h"

#include "state_db/delta_batch.h"

namespace scs
{

void 
ExecutionContext::invoke_subroutine(MethodInvocation const& invocation)
{
	auto iter = active_runtimes.find(invocation.addr);
	if (iter == active_runtimes.end())
	{
		CONTRACT_INFO("creating new runtime for contract at %s", debug::array_to_str(invocation.addr).c_str());

		auto runtime_instance = wasm_context.new_runtime_instance(invocation.addr, static_cast<const void*>(&(tx_context->get_src_tx_hash())));
		if (!runtime_instance)
		{
			throw wasm_api::HostError("cannot find target address");
		}

		active_runtimes.emplace(invocation.addr, std::move(runtime_instance));
		BuiltinFns::link_fns(*active_runtimes.at(invocation.addr));
	}

	auto* runtime = active_runtimes.at(invocation.addr).get();

	tx_context -> push_invocation_stack(runtime, invocation);

	runtime->template invoke<void>(invocation.get_invocable_methodname().c_str());

	tx_context -> pop_invocation_stack();
}

TransactionStatus
ExecutionContext::execute(Hash const& tx_hash, Transaction const& tx, TxBlock& txs, DeltaBatch& delta_batch)
{
	if (executed)
	{
		throw std::runtime_error("can't reuse execution context!");
	}
	executed = true;

	MethodInvocation invocation(tx.invocation);

	tx_context = std::make_unique<TransactionContext>(
		tx.gas_limit, 
		tx.gas_rate_bid,
		tx_hash, 
		tx.sender, 
		scs_data_structures,
		delta_batch.get_serial_subsidiary());

	try
	{
		invoke_subroutine(invocation);
	} 
	catch(wasm_api::WasmError& e)
	{
		CONTRACT_INFO("Execution error: %s", e.what());
		txs.template invalidate<TransactionFailurePoint::COMPUTE>(tx_hash);
		return TransactionStatus::FAILURE;
	}
	catch(...)
	{
		std::printf("unrecoverable error!\n");
		std::abort();
	}

	tx_context->storage_proxy.push_deltas_to_batch();

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
