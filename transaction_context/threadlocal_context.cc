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

void 
ThreadlocalContextStore::make_ctx(std::unique_ptr<WasmContext>&& c)
{
	ctx = std::unique_ptr<ExecutionContext>(new ExecutionContext(std::move(c)));
}

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
