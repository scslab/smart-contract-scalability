#pragma once

#include <cstdint>
#include <optional>
#include <memory>

#include "transaction_context/transaction_context.h"

#include "wasm_api/wasm_api.h"

namespace scs
{

struct BuiltinFnWrappers
{
	static void 
	builtin_scs_return(int32_t offset, int32_t len);

	static void
	builtin_scs_invoke(
		int32_t addr_offset, 
		int32_t methodname_offset, 
		int32_t methodname_len, 
		int32_t calldata_offset, 
		int32_t calldata_len);
};

class ExecutionContext {

	std::unique_ptr<WasmContext> wasm_context;

	std::map<Address, std::unique_ptr<WasmRuntime>> active_runtimes;

	//TransactionStateDeltaBatch tx_state_delta;

	std::unique_ptr<TransactionContext> tx_context;

	bool executed;

	void link_builtin_fns(WasmRuntime& runtime);

	friend class ThreadlocalExecutionContext;

	ExecutionContext(std::unique_ptr<WasmContext> ctx)
		: wasm_context(std::move(ctx))
		, active_runtimes()
		//, tx_state_delta()
		, tx_context(nullptr)
		, executed(false)
		{}

	friend class BuiltinFnWrappers;

	// should only be used by builtin fns
	void invoke_subroutine(MethodInvocation invocation);

	TransactionContext& get_transaction_context()
	{
		return *tx_context;
	}

public:

	TransactionStatus
	execute(MethodInvocation const& invocation, uint64_t gas_limit, Address const& src);

	void reset();
};

class ThreadlocalExecutionContext {
	inline static thread_local std::unique_ptr<ExecutionContext> ctx;

	ThreadlocalExecutionContext() = delete;

public:
	static ExecutionContext& get_ctx();
	static void make_ctx(std::unique_ptr<WasmContext>&& c);
};

} /* scs */
