#pragma once

#include "xdr/block.h"

#include <atomic>
#include <tbb/blocked_range.h>

#include "transaction_context/execution_context.h"

#include <utils/threadlocal_cache.h>

namespace scs {

template<typename GlobalContext_t, typename BlockContext_t>
struct ValidateReduce
{
    std::atomic<bool>& found_error;
    GlobalContext_t& global_context;
    BlockContext_t& block_context;
    Block const& txs;

    using TransactionContext_t = typename BlockContext_t::tx_context_t;

    utils::ThreadlocalCache<ExecutionContext<TransactionContext_t>, TLCACHE_SIZE>& execs;

    using TxContext_t = typename BlockContext_t::tx_context_t;

    void operator()(const tbb::blocked_range<std::size_t> r)
    {
        if (found_error)
            return;

        // TBB docs suggest this type of pattern (use local var until end)
        //  optimizes better.
        bool local_found_error = false;

        auto& exec_ctx = execs.get(global_context.engine);

        for (size_t i = r.begin(); i < r.end(); i++) {

            auto const& txset_entry = txs.transactions[i];
            auto const& tx = txset_entry.tx;

            auto hash = hash_xdr(tx);

            for (size_t j = 0; j < txset_entry.nondeterministic_results.size();
                 j++) {
                auto status
                    = exec_ctx.execute(hash,
                                       tx,
                                       global_context,
                                       block_context,
                                       txset_entry.nondeterministic_results[j]);

                if (status != TransactionStatus::SUCCESS) {
                    local_found_error = true;
                    break;
                }
            }
        } 

        found_error = found_error || local_found_error;
    }

    // split constructor can be concurrent with operator()
    ValidateReduce(ValidateReduce& x, tbb::split)
        : found_error(x.found_error)
        , global_context(x.global_context)
        , block_context(x.block_context)
        , txs(x.txs)
        , execs(x.execs) {};

    void join(ValidateReduce& other) {}

    ValidateReduce(std::atomic<bool>& found_error,
                   GlobalContext_t& global_context,
                   BlockContext_t& block_context,
                   Block const& txs,
                   utils::ThreadlocalCache<ExecutionContext<TransactionContext_t>, TLCACHE_SIZE>& execs)
        : found_error(found_error)
        , global_context(global_context)
        , block_context(block_context)
        , txs(txs)
        , execs(execs)
    {}
};

} // namespace scs
