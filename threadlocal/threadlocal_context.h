#pragma once

#include <memory>

#include <utils/threadlocal_cache.h>

#include "transaction_context/execution_context.h"

#include "threadlocal/gc.h"
#include "threadlocal/uid.h"

namespace scs {

class ExecutionContext;

class ThreadlocalContextStore
{
    struct context_t
    {
        std::unique_ptr<ExecutionContext> ctx;

        MultitypeGarbageCollector<StorageObject>
            gc;

        UniqueIdentifier uid;
    };

    inline static utils::ThreadlocalCache<context_t> cache;

    ThreadlocalContextStore() = delete;

  public:
    static ExecutionContext& get_exec_ctx();

    template<typename delete_t>
    static void defer_delete(const delete_t* ptr)
    {
        (cache.get()).gc.template deferred_delete<delete_t>(ptr);
    }

    static uint64_t get_uid();

    template<typename... Args>
    static void make_ctx(Args&... args);

    static void post_block_clear();

    // for testing
    static void clear_entire_context();
};

namespace test {

struct DeferredContextClear
{
    ~DeferredContextClear() { ThreadlocalContextStore::clear_entire_context(); }
};

} // namespace test

} // namespace scs
