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

	static int32_t
	builtin_scs_invoke(
		int32_t addr_offset, 
		int32_t methodname, 
		int32_t calldata_offset, 
		int32_t calldata_len);

	static int32_t
	builtin_scs_invoke_with_return(
		int32_t addr_offset, 
		int32_t methodname, 
		int32_t calldata_offset, 
		int32_t calldata_len,
		int32_t return_offset,
		int32_t return_len);

	static void
	builtin_scs_log(
		int32_t log_offset,
		int32_t log_len);
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
	int32_t invoke_subroutine(MethodInvocation invocation);

	TransactionContext& get_transaction_context();

public:

	TransactionStatus
	execute(MethodInvocation const& invocation, uint64_t gas_limit);

	void reset();

	std::vector<std::vector<uint8_t>> const&
	get_logs();
};

class ThreadlocalExecutionContext {
	inline static thread_local std::unique_ptr<ExecutionContext> ctx;

	ThreadlocalExecutionContext() = delete;

public:
	static ExecutionContext& get_ctx();
	static void make_ctx(std::unique_ptr<WasmContext>&& c);
};

} /* scs */
