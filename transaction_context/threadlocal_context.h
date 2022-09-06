#pragma once

#include <memory>

#include "mtt/utils/threadlocal_cache.h"

#include "transaction_context/threadlocal_types.h"

#include "transaction_context/execution_context.h"

#include "threadlocal/gc.h"

namespace scs {

class ExecutionContext;

class ThreadlocalContextStore
{
    struct context_t
    {
        std::unique_ptr<ExecutionContext> ctx;

        //GarbageCollector<xdr::opaque_vec<RAW_MEMORY_MAX_LEN>> mem_vec_gc;
        MultitypeGarbageCollector<uint64_t, xdr::opaque_vec<RAW_MEMORY_MAX_LEN>> gc;
    };

    static utils::ThreadlocalCache<context_t> cache;

    //inline static thread_local std::unique_ptr<ExecutionContext> ctx;

    ThreadlocalContextStore() = delete;

  public:
    static ExecutionContext& get_exec_ctx();


    template<typename delete_t>
    static void defer_delete(const delete_t* ptr)
    {
        (cache.get()).gc.template deferred_delete<delete_t>(ptr);
    }

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
