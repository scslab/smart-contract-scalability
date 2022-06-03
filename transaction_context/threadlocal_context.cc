#include "transaction_context/threadlocal_context.h"

#include "transaction_context/execution_context.h"

#include "wasm_api/wasm_api.h"

#include "transaction_context/global_context.h"

namespace scs
{

ExecutionContext&  
ThreadlocalContextStore::get_exec_ctx()
{
	return *ctx;
}

template<typename ...Args>
void 
ThreadlocalContextStore::make_ctx(Args&... args)
{
	ctx = std::unique_ptr<ExecutionContext>(new ExecutionContext(args...));
}

template
void ThreadlocalContextStore::make_ctx(GlobalContext&);

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

void
ThreadlocalContextStore::clear_entire_context()
{
	delta_batches.clear();
	ctx = nullptr;
}


} /* scs */
