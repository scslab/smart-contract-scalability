#pragma once

#include <memory>

#include <utils/threadlocal_cache.h>

#include "transaction_context/execution_context.h"

#include "threadlocal/gc.h"
#include "threadlocal/uid.h"
#include "threadlocal/allocator.h"
#include "threadlocal/timeout.h"

#include "xdr/storage.h"

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

        Timeout timeout;
    };

    inline static utils::ThreadlocalCache<context_t> cache;

    inline static BlockAllocator<HashSetEntry> hash_allocator;

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

    static Timeout& get_timer() {
        return cache.get().timeout;
    }

    static uint64_t get_uid();

    template<typename... Args>
    static void make_ctx(Args&... args);

    static void post_block_clear();

    static void post_block_timeout();

    static void clear_entire_context();
};

namespace test {

struct DeferredContextClear
{
    ~DeferredContextClear() { ThreadlocalContextStore::clear_entire_context(); }
};

} // namespace test

} // namespace scs
