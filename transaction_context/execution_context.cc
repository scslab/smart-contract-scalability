#include "transaction_context/execution_context.h"

namespace scs
{

void 
BuiltinFnWrappers::builtin_scs_return(int32_t offset, int32_t len)
{
	auto& tx_ctx = ThreadlocalExecutionContext::get_ctx().get_transaction_context();

	tx_ctx.return_buf = tx_ctx.runtime_stack.back().template load_from_memory<std::vector<uint8_t>>(offset, len);
}

void 
BuiltinFnWrappers::builtin_scs_invoke(
	int32_t addr_offset, 
	int32_t methodname_offset, 
	int32_t methodname_len, 
	int32_t calldata_offset, 
	int32_t calldata_len)
{
	auto& tx_ctx = ThreadlocalExecutionContext::get_ctx().get_transaction_context();

	auto& runtime = tx_ctx.runtime_stack.back();

	MethodInvocation invocation
	{

		.addr = runtime.template load_from_memory_to_const_size_buf<Address>(addr_offset),
		.method_name = runtime.template load_from_memory<std::vector<uint8_t>>(methodname_offset, methodname_len),
		.calldata = runtime.template load_from_memory<std::vector<uint8_t>>(calldata_offset, calldata_len)
	};

	ThreadlocalExecutionContext::get_ctx().invoke_subroutine(invocation);
	tx_ctx.return_buf.clear();
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

	/*runtime.link_fn(
		"scs", 
		"invoke", 
		[this, &runtime] (
			uint32_t addr_offset, 
			uint32_t methodname_offset, 
			uint32_t methodname_len, 
			uint32_t calldata_offset, 
			uint32_t calldata_len,
			uint32_t return_offset,
			uint32_t return_len)
		-> void
	{
		TransactionInvocation invocation
		{
			.invokedAddress = runtime.template load_from_memory<Address>(addr_offset, 32),
			.methodName = runtime.template load_from_memory<std::string>(methodname_offset, methodname_len),
			.calldata = runtime.template load_from_memory<uint8_t>(calldata_offset, calldata_len)
		};

		invoke_subroutine(invocation);

		runtime.write_to_memory(tx_context.return_buf, return_offset, return_len);
		tx_context.return_buf.clear();
	});*/
}

void 
ExecutionContext::invoke_subroutine(MethodInvocation invocation)
{
	tx_context->invocation_stack.push_back(invocation);

	auto iter = active_runtimes.find(invocation.addr);
	if (iter == active_runtimes.end())
	{
		active_runtimes.emplace(invocation.addr, wasm_context->new_runtime_instance(invocation.addr));
		link_builtin_fns(*active_runtimes.at(invocation.addr));
	}

	active_runtimes.at(invocation.addr)->invoke(invocation);
	tx_context->invocation_stack.pop_back();
}

TransactionStatus
ExecutionContext::execute(MethodInvocation const& invocation, uint64_t gas_limit, Address const& src)
{
	if (executed)
	{
		throw std::runtime_error("can't reuse execution context!");
	}
	executed = true;

	tx_context = std::make_unique<TransactionContext>(gas_limit, src);

	try
	{
		invoke_subroutine(invocation);
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
