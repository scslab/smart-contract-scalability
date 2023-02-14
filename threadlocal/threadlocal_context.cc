/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#if USE_RPC
std::optional<RpcResult>
ThreadlocalContextStore::send_cancellable_rpc(std::unique_ptr<ExternalCall::Stub>
const& stub, RpcCall const& call)
{
    auto& ctx = cache.get();
    uint64_t uid = ctx.uid.get();

    return ctx.rpc.send_query(call, uid, stub);
}
#endif

void
ThreadlocalContextStore::clear_entire_context()
{
    cache.clear();
    hash_allocator.reset();
}

} // namespace scs
