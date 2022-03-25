#pragma once

#include "wasm_api/wasm_api.h"

namespace scs
{

class BuiltinFnWrappers
{
	static void 
	builtin_scs_return(uint32_t offset, uint32_t len);

	static void
	builtin_scs_invoke(
		uint32_t addr_offset, 
		uint32_t methodname_offset, 
		uint32_t methodname_len, 
		uint32_t calldata_offset, 
		uint32_t calldata_len);
};

class ExecutionContext {

	WasmContext wasm_context;

	std::map<Address, std::unique_ptr<WasmRuntime>> active_runtimes;

	TransactionStateDeltaBatch tx_state_delta;

	std::optional<TransactionContext> tx_context;

	bool executed;

	void link_builtin_fns(WasmRuntime& runtime);

	ExecutionContext(WasmContext&& ctx)
		: wasm_context(std::move(ctx))
		, active_runtiems()
		, tx_state_delta()
		, tx_context(std::nullopt)
		, executed(false)
		{}

	friend class BuiltinFnWrappers;
	// should only be used by builtin fns
	void invoke_subroutine(TransactionInvocation invocation);

	TransactionContext& get_tx_context() 
	{
		return (*tx_context).runtime_stack.back();
	}

public:

	TransactionStatus
	execute(TransactionInvocation const& invocation_context, uint64_t gas_limit, Address src);

	void reset();
};

class ThreadlocalExecutionContext {
	static thread_local std::unique_ptr<ExecutionContext> ctx;

public:
	static ExecutionContext& get_ctx()
	{
		return *ctx;
	}

	static void make_ctx()
	{
		ctx = std::make_unique<
	}
};

} /* scs */
