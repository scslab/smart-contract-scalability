#pragma once

#include <cstdint>
#include <optional>
#include <memory>

#include "transaction_context/transaction_context.h"

#include <wasm_api/wasm_api.h>

#include "xdr/transaction.h"

#include "transaction_context/global_context.h"

namespace scs
{

class ExecutionContext {

	wasm_api::WasmContext wasm_context;
	GlobalContext const& scs_data_structures;

	std::map<Address, std::unique_ptr<wasm_api::WasmRuntime>> active_runtimes;

	std::unique_ptr<TransactionContext> tx_context;

	bool executed;

	ExecutionContext(GlobalContext const& scs_data_structures)
		: wasm_context(scs_data_structures.contract_db, MAX_STACK_BYTES)
		, scs_data_structures(scs_data_structures)
		, active_runtimes()
		, tx_context(nullptr)
		, executed(false)
		{}

	friend class ThreadlocalContextStore;
	friend class BuiltinFns;

	// should only be used by builtin fns
	void invoke_subroutine(MethodInvocation const& invocation);

	TransactionContext& get_transaction_context();

public:

	TransactionStatus
	execute(Transaction const& invocation);

	void reset();

	std::vector<std::vector<uint8_t>> const&
	get_logs();
};

} /* scs */
