#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"

#include "wasm_api/wasm_api.h"

#include "transaction_context/global_context.h"

//#include "proto/external_call.pb.h"

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
    auto& ctx = cache.get().ctx;
    if (!ctx) {
        ctx = std::unique_ptr<ExecutionContext>(new ExecutionContext(args...));
    }
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
            if (c.ctx) {
                c.ctx->reset();
            }
            c.gc.post_block_clear();
            // no need to reset uid, we're not going to overflow 2^48
        }
    }

    hash_allocator.reset();
}

void
ThreadlocalContextStore::stop_rpcs()
{
    auto& ctxs = cache.get_objects();
    for (auto& ctx : ctxs) {
        if (ctx) {
             ctx -> rpc.cancel_and_set_disallowed();
        }
    }
}

void
ThreadlocalContextStore::enable_rpcs()
{
    auto& ctxs = cache.get_objects();
    for (auto& ctx : ctxs) {
        if (ctx) {
             ctx -> rpc.set_allowed();
        }
    }
}


std::optional<RpcResult>
ThreadlocalContextStore::send_cancellable_rpc(std::unique_ptr<ExternalCall::Stub>
const& stub, RpcCall const& call)
{
    auto& ctx = cache.get();
    uint64_t uid = ctx.uid.get();

    return ctx.rpc.send_query(call, uid, stub);
}

void
ThreadlocalContextStore::clear_entire_context()
{
    cache.clear();
    hash_allocator.reset();
}

} // namespace scs
