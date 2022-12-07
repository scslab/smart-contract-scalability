#pragma once

#include <memory>

#include <utils/threadlocal_cache.h>

#include "transaction_context/execution_context.h"

#include "threadlocal/gc.h"
#include "threadlocal/uid.h"
#include "threadlocal/allocator.h"
#include "threadlocal/timeout.h"
#include "threadlocal/rate_limiter.h"
#include "threadlocal/cancellable_rpc.h"

#include "xdr/storage.h"

#include "proto/external_call.grpc.pb.h"

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

        bool has_limiter_slot = false;

        context_t()
            : ctx()
            , gc()
            , uid()
            , rpc()
            , has_limiter_slot(false)
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

    static std::optional<RpcResult>
    send_cancellable_rpc(std::unique_ptr<ExternalCall::Stub> const& stub, RpcCall const& call);

   /* static Notifyable prep_timer_for(uint64_t request_id)
    {
        return (cache.get()).timeout.prep_timer_for(request_id);
    }

    static auto timer_await(uint64_t request_id)
    {
        rate_limiter_free_slot();

        return cache.get().timeout.await(request_id);
    } */

   // static void rate_limiter_notify_stop()
   // {
   //     rate_limiter_free_slot();
   //     rate_limiter.notify_unslotted_stopped_running();
   // }

    static void rate_limiter_free_slot()
    {
        if (cache.get().has_limiter_slot)
        {
            rate_limiter.free_one_slot();
            cache.get().has_limiter_slot = false;
        }
    }
/*
    static void rate_limiter_notify_stop_thread_if_unslotted()
    {
        if (!cache.get().has_limiter_slot)
        {
            rate_limiter.notify_unslotted_stopped_running();
        }
    } */

    static 
    bool rate_limiter_wait_for_opening()
    {
        bool is_shutdown;

        if (cache.get().has_limiter_slot)
        {
            is_shutdown = rate_limiter.fastpath_wait_for_opening();
        } 
        else
        {
            is_shutdown = rate_limiter.wait_for_opening();
        }

        if (!is_shutdown)
        {
            cache.get().has_limiter_slot = true;
        }
        return is_shutdown;
    }

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

   // static void join_all_threads();

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
