#pragma once

#include <memory>

#include <utils/threadlocal_cache.h>

#include "transaction_context/execution_context.h"

#include "threadlocal/gc.h"
#include "threadlocal/uid.h"
#include "threadlocal/allocator.h"
#include "threadlocal/rate_limiter.h"
#include "threadlocal/cancellable_rpc.h"

#include "xdr/storage.h"

#include "config.h"

#if USE_RPC
    #include "proto/external_call.grpc.pb.h"
#endif

namespace scs {

class ExecutionContext;

class ThreadlocalContextStore
{
    struct context_t
    {
        std::unique_ptr<ExecutionContext> ctx;

        MultitypeGarbageCollector<StorageDeltaClass>
            gc;

        UniqueIdentifier uid;

        CancellableRPC rpc;

        context_t()
            : ctx()
            , gc()
            , uid()
            , rpc()
            {}
    };

    inline static utils::ThreadlocalCache<context_t> cache;

    inline static BlockAllocator<HashSetEntry> hash_allocator;

    inline static RateLimiter rate_limiter;

    ThreadlocalContextStore() = delete;

  public:
    static ExecutionContext& get_exec_ctx();

    template<typename delete_t>
    static void defer_delete(const delete_t* ptr)
    {
        (cache.get()).gc.template deferred_delete<delete_t>(ptr);
    }

    static HashSetEntry const& 
    get_hash(uint32_t idx)
    {
        return hash_allocator.get(idx);
    }

    static uint32_t 
    allocate_hash(HashSetEntry&& h)
    {
        return hash_allocator.allocate(std::move(h));
    }

    #if USE_RPC
    static std::optional<RpcResult>
    send_cancellable_rpc(std::unique_ptr<ExternalCall::Stub> const& stub, RpcCall const& call);
    #endif

    static auto& get_rate_limiter()
    {
        return rate_limiter;
    }

    static uint64_t get_uid();

    template<typename... Args>
    static void make_ctx(Args&... args);

    static void enable_rpcs();
    static void stop_rpcs();

    static void post_block_clear();

    static void clear_entire_context();
};

namespace test {

struct DeferredContextClear
{
    ~DeferredContextClear() { 
        ThreadlocalContextStore::clear_entire_context();
    }
};

} // namespace test

} // namespace scs
