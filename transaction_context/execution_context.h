#pragma once

#include <cstdint>
#include <optional>
#include <memory>

#include "transaction_context/transaction_context.h"

#include "wasm_api/wasm_api.h"

#include "xdr/transaction.h"

namespace scs
{

class ExecutionContext {

	std::unique_ptr<WasmContext> wasm_context;

	std::map<Address, std::unique_ptr<WasmRuntime>> active_runtimes;

	std::unique_ptr<TransactionContext> tx_context;

	bool executed;

	//void link_builtin_fns(WasmRuntime& runtime);


	ExecutionContext(std::unique_ptr<WasmContext> ctx)
		: wasm_context(std::move(ctx))
		, active_runtimes()
		//, tx_state_delta()
		, tx_context(nullptr)
		, executed(false)
		{}

	friend class ThreadlocalContextStore;
	friend class BuiltinFns;

	// should only be used by builtin fns
	void invoke_subroutine(MethodInvocation invocation);

	TransactionContext& get_transaction_context();

public:

	TransactionStatus
	execute(Transaction const& invocation);

	void reset();

	std::vector<std::vector<uint8_t>> const&
	get_logs();
};

} /* scs */
