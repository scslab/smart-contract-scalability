#include "transaction_context/threadlocal_context.h"

#include "transaction_context/execution_context.h"

#include "wasm_api/wasm_api.h"

namespace scs
{

ExecutionContext&  
ThreadlocalContextStore::get_exec_ctx()
{
	return *ctx;
}

template<typename WasmContext_T, typename ...Args>
requires std::derived_from<WasmContext_T, WasmContext>
void 
ThreadlocalContextStore::make_ctx(Args&... args)
{
	ctx = std::unique_ptr<ExecutionContext>(new ExecutionContext(new WasmContext_T(args...)));
}

template
void ThreadlocalContextStore::make_ctx<Wasm3_WasmContext>(ContractDB& db);

SerialDeltaBatch&
ThreadlocalContextStore::get_delta_batch()
{
	return delta_batches.get();
}

void
ThreadlocalContextStore::post_block_clear()
{
	delta_batches.clear();
	ctx -> reset();
}


} /* scs */
