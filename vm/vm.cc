#include "vm/vm.h"

#include <tbb/parallel_reduce.h>

#include <atomic>

#include "crypto/hash.h"
#include "phase/phases.h"
#include "threadlocal/threadlocal_context.h"

namespace scs {

struct ValidateReduce
{
    std::atomic<bool>& found_error;
    GlobalContext& global_context;
    BlockContext& block_context;
    std::vector<SignedTransaction> const& txs;

    void operator()(const tbb::blocked_range<std::size_t> r)
    {
        if (found_error)
            return;

        ThreadlocalContextStore::make_ctx(global_context);

        // TBB docs suggest this type of pattern (use local var until end)
        //  optimizes better.
        bool local_found_error = false;

        for (size_t i = r.begin(); i < r.end(); i++) {

            auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

            auto hash = hash_xdr(txs[i]);

            auto status = exec_ctx.execute(hash, txs[i], block_context);

            if (status != TransactionStatus::SUCCESS) {
                local_found_error = true;
                break;
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
                   GlobalContext& global_context,
                   BlockContext& block_context,
                   std::vector<SignedTransaction> const& txs)
        : found_error(found_error)
        , global_context(global_context)
        , block_context(block_context)
        , txs(txs)
    {}
};

bool
VirtualMachine::validate_tx_block(std::vector<SignedTransaction> const& txs)
{
    std::atomic<bool> found_error = false;

    ValidateReduce reduce(
        found_error, global_context, *current_block_context, txs);

    tbb::parallel_reduce(tbb::blocked_range<size_t>(0, txs.size()), reduce);

    return !found_error;
}

void
VirtualMachine::advance_block_number()
{
    current_block_context = std::make_unique<BlockContext>(
        current_block_context->block_number + 1);
}

std::optional<BlockHeader>
VirtualMachine::try_exec_tx_block(std::vector<SignedTransaction> const& txs)
{
    auto res = validate_tx_block(txs);

    if (!res) {
        phase_undo_block(global_context, *current_block_context);
        advance_block_number();
        return std::nullopt;
    } 

    phase_finish_block(global_context, *current_block_context);

    // now time for hashing a block header
    BlockHeader out;

    out.block_number = current_block_context -> block_number;
    out.prev_header_hash = prev_block_hash;
    out.tx_set_hash = current_block_context -> tx_set.hash();
    out.modified_keys_hash = current_block_context -> modified_keys_list.hash();
    out.state_db_hash = global_context.state_db.hash();
    out.contract_db_hash = global_context.contract_db.hash();

    advance_block_number();
    return out;
}

} // namespace scs
