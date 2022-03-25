#include "wasm_api/wasm_api.h"

namespace scs
{

std::unique_ptr<WasmRuntime> 
Wasm3_WasmContext::new_runtime_instance(Address const& contract_address)
{
	Contract const& contract = *contract_db.get_contract(contract_address);

	auto module = env.parse_module(contract.data(), contract.size());

	auto runtime = env.new_runtime(MAX_STACK_BYTES);

	runtime.load(module);

	return std::make_unique<Wasm3_WasmRuntime>(std::move(runtime), std::move(module), *this);
}

TransactionStatus 
Wasm3_WasmRuntime::invoke(TransactionInvocation const& tx)
{
	throw std::runtime_error("unimpl");
}



} /* scs */
