#pragma once

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

	TransactionInvocation invocation
	{
		.invokedAddress = runtime.template load_from_memory<Address>(addr_offset, 32),
		.methodName = runtime.template load_from_memory<std::string>(methodname_offset, methodname_len),
		.calldata = runtime.template load_from_memory<std::vector<uint8_t>>(calldata_offset, calldata_len)
	};

	ThreadlocalExecutionContext::get_ctx().invoke_subroutine(invocation);
	tx_context.return_buf.clear();
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
ExecutionContext::invoke_subroutine(TransactionInvocation invocation)
{
	tx_context.invocation_stack.push_back(invocation);

	auto iter = active_runtimes.find(invocation.invokedAddress);
	if (iter == active_runtimes.end())
	{
		active_runtimes.emplace(invocation.invokedAddress, wasm_context.new_runtime_instance(invocation.invokedAddress));
		link_builtin_fns(active_runtimes.at(invocation.invokedAddress));
	}

	active_runtimes.at(invocation.invokedAddress).invoke(invocation);
	tx_context.invocation_stack.pop_back();
}

TransactionStatus
ExecutionContext::execute(TransactionInvocation const& invocation_context, uint64_t gas_limit, Address src);
{
	if (executed)
	{
		throw std::runtime_error("can't reuse execution context!");
	}
	executed = true;

	tx_context = std::make_optional<TransactionContext>(src, gas_limit);

	try
	{
		invoke_subroutine(tx_context);
	} 
	catch(const std::exception& e)
	{
		std::printf("Execution error: %s\n", e.what().c_str());
		return TransactionStatus::FAILURE;
	}
}

void
ExecutionContext::reset()
{
	// nothing to do to clear wasm_context
	active_runtimes.clear();
	tx_state_delta.clear();
	tx_context = std::nullopt;
	executed = false;
}



} /* scs */
