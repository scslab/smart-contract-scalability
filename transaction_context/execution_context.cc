#include "transaction_context/execution_context.h"

#include "debug/debug_macros.h"

namespace scs
{

void 
BuiltinFnWrappers::builtin_scs_return(int32_t offset, int32_t len)
{
	auto& tx_ctx = ThreadlocalExecutionContext::get_ctx().get_transaction_context();

	tx_ctx.return_buf = tx_ctx.runtime_stack.back()->template load_from_memory<std::vector<uint8_t>>(offset, len);
}

void 
BuiltinFnWrappers::builtin_scs_get_calldata(int32_t offset, int32_t len)
{
	auto& tx_ctx = ThreadlocalExecutionContext::get_ctx().get_transaction_context();

	auto& calldata = tx_ctx.invocation_stack.back().calldata;

	tx_ctx.runtime_stack.back() -> write_to_memory(calldata, offset, len);
}


void 
BuiltinFnWrappers::builtin_scs_invoke(
	int32_t addr_offset, 
	int32_t methodname, 
	int32_t calldata_offset, 
	int32_t calldata_len,
	int32_t return_offset,
	int32_t return_len)
{
	auto& tx_ctx = ThreadlocalExecutionContext::get_ctx().get_transaction_context();

	auto& runtime = *tx_ctx.runtime_stack.back();

	MethodInvocation invocation
	{
		.addr = runtime.template load_from_memory_to_const_size_buf<Address>(addr_offset),
		.method_name = static_cast<uint32_t>(methodname),
		.calldata = runtime.template load_from_memory<std::vector<uint8_t>>(calldata_offset, calldata_len)
	};

	if (return_len > 0)
	{
		runtime.write_to_memory(tx_ctx.return_buf, return_offset, return_len);
	}

	ThreadlocalExecutionContext::get_ctx().invoke_subroutine(invocation);
	tx_ctx.return_buf.clear();
}

void
BuiltinFnWrappers::builtin_scs_log(
	int32_t log_offset,
	int32_t log_len)
{
	auto& tx_ctx = ThreadlocalExecutionContext::get_ctx().get_transaction_context();

	auto& runtime = *tx_ctx.runtime_stack.back();

	auto log = runtime.template load_from_memory<std::vector<uint8_t>>(log_offset, log_len);

	tx_ctx.logs.push_back(log);
}

void 
ExecutionContext::link_builtin_fns(WasmRuntime& runtime)
{
	//TODO

	runtime.link_fn(
		"scs", 
		"invoke", 
		&BuiltinFnWrappers::builtin_scs_invoke);

	runtime.link_fn(
		"scs",
		"return",
		&BuiltinFnWrappers::builtin_scs_return);

	runtime.link_fn(
		"scs",
		"get_calldata",
		&BuiltinFnWrappers::builtin_scs_get_calldata);

	runtime.link_fn(
		"scs",
		"host_log",
		&BuiltinFnWrappers::builtin_scs_log);
}

void 
ExecutionContext::invoke_subroutine(MethodInvocation invocation)
{
	tx_context->invocation_stack.push_back(invocation);

	auto iter = active_runtimes.find(invocation.addr);
	if (iter == active_runtimes.end())
	{
		CONTRACT_INFO("creating new runtime for contract at %s", debug::array_to_str(invocation.addr).c_str());
		active_runtimes.emplace(invocation.addr, wasm_context->new_runtime_instance(invocation.addr));
		link_builtin_fns(*active_runtimes.at(invocation.addr));
	}

	auto* runtime = active_runtimes.at(invocation.addr).get();

	tx_context -> runtime_stack.push_back(runtime);

	runtime->invoke(invocation);

	tx_context -> runtime_stack.pop_back();

	tx_context->invocation_stack.pop_back();
	return res;
}

TransactionStatus
ExecutionContext::execute(MethodInvocation const& invocation, uint64_t gas_limit)
{
	if (executed)
	{
		throw std::runtime_error("can't reuse execution context!");
	}
	executed = true;

	tx_context = std::make_unique<TransactionContext>(gas_limit, invocation.addr);

	try
	{
		int32_t res = invoke_subroutine(invocation);
		if (res < 0) {
			throw std::runtime_error("contract returned error");
		}
	} 
	catch(const std::exception& e)
	{
		std::printf("Execution error: %s\n", e.what());
		return TransactionStatus::FAILURE;
	}

	return TransactionStatus::SUCCESS;
}

void
ExecutionContext::reset()
{
	// nothing to do to clear wasm_context
	active_runtimes.clear();
	//tx_state_delta.clear();
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

ExecutionContext&  
ThreadlocalExecutionContext::get_ctx()
{
	return *ctx;
}

void 
ThreadlocalExecutionContext::make_ctx(std::unique_ptr<WasmContext>&& c)
{
	ctx = std::unique_ptr<ExecutionContext>(new ExecutionContext(std::move(c)));
}

} /* scs */
