#pragma once

#include "xdr/block.h"

#include <atomic>
#include <tbb/blocked_range.h>

#include "transaction_context/execution_context.h"
#include "threadlocal/threadlocal_context.h"

namespace scs {

template<typename GlobalContext_t, typename BlockContext_t>
struct ValidateReduce
{
    std::atomic<bool>& found_error;
    GlobalContext_t& global_context;
    BlockContext_t& block_context;
    Block const& txs;

    using TxContext_t = typename BlockContext_t::tx_context_t;

    void operator()(const tbb::blocked_range<std::size_t> r)
    {
        if (found_error)
            return;

        ThreadlocalTransactionContextStore<TxContext_t>::make_ctx();

        // TBB docs suggest this type of pattern (use local var until end)
        //  optimizes better.
        bool local_found_error = false;

        for (size_t i = r.begin(); i < r.end(); i++) {

            auto& exec_ctx = ThreadlocalTransactionContextStore<
                TxContext_t>::get_exec_ctx();

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
        , txs(x.txs){};

    void join(ValidateReduce& other) {}

    ValidateReduce(std::atomic<bool>& found_error,
                   GlobalContext_t& global_context,
                   BlockContext_t& block_context,
                   Block const& txs)
        : found_error(found_error)
        , global_context(global_context)
        , block_context(block_context)
        , txs(txs)
    {}
};

} // namespace scs
