#include "transaction_context/threadlocal_context.h"

#include "transaction_context/execution_context.h"

#include "wasm_api/wasm_api.h"

#include "transaction_context/global_context.h"

namespace scs {

ExecutionContext&
ThreadlocalContextStore::get_exec_ctx()
{
    return *(cache.get().ctx);
    //return *ctx;
}

template<typename... Args>
void
ThreadlocalContextStore::make_ctx(Args&... args)
{
    (cache.get().ctx) = std::unique_ptr<ExecutionContext>(new ExecutionContext(args...));
}

template void
ThreadlocalContextStore::make_ctx(GlobalContext&);

void
ThreadlocalContextStore::post_block_clear()
{
    auto& ctxs = cache.get_objects();
    for (auto& ctx : ctxs)
    {
        if (ctx)
        {
            (*ctx).ctx->reset();
            (*ctx).gc.post_block_clear();
        }
    }
    //ctx->reset();
}

void
ThreadlocalContextStore::clear_entire_context()
{
    cache.clear();
    /*
    auto& ctxs = cache.get_objects();
    for (auto& ctx : ctxs)
    {
        if (ctx)
        {
            (*ctx).ctx = nullptr;
            (*ctx).gc.post_block_clear();
        }
    } */
    //ctx = nullptr;
}

} // namespace scs
