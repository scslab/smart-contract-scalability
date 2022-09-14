#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"

#include "wasm_api/wasm_api.h"

#include "transaction_context/global_context.h"

namespace scs {

ExecutionContext&
ThreadlocalContextStore::get_exec_ctx()
{
    return *(cache.get().ctx);
}

uint64_t
ThreadlocalContextStore::get_uid()
{
    return cache.get().uid.get();
}

template<typename... Args>
void
ThreadlocalContextStore::make_ctx(Args&... args)
{
    (cache.get().ctx)
        = std::unique_ptr<ExecutionContext>(new ExecutionContext(args...));
}

template void
ThreadlocalContextStore::make_ctx(GlobalContext&);

void
ThreadlocalContextStore::post_block_clear()
{
    auto& ctxs = cache.get_objects();
    for (auto& ctx : ctxs) {
        if (ctx) {
            auto& c = *ctx;
            c.ctx->reset();
            c.gc.post_block_clear();
            // no need to reset uid, we're not going to overflow 2^48
        }
    }
}

void
ThreadlocalContextStore::clear_entire_context()
{
    cache.clear();
}

} // namespace scs
