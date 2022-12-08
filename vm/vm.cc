#include "vm/vm.h"

#include <tbb/parallel_reduce.h>

#include <atomic>

#include "crypto/hash.h"
#include "phase/phases.h"
#include "threadlocal/threadlocal_context.h"

#include "vm/genesis.h"

#include "block_assembly/limits.h"
#include "block_assembly/assembly_worker.h"

namespace scs {

void
VirtualMachine::init_default_genesis()
{
    install_genesis_contracts(global_context.contract_db);
    current_block_context = std::make_unique<BlockContext>(0);
}

void
VirtualMachine::assert_initialized() const
{
    if (!current_block_context)
    {
        throw std::runtime_error("uninitialized");
    }
}

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

BlockHeader
VirtualMachine::make_block_header() 
{
    BlockHeader out;

    out.block_number = current_block_context -> block_number;
    out.prev_header_hash = prev_block_hash;
    out.tx_set_hash = current_block_context -> tx_set.hash();
    out.modified_keys_hash = current_block_context -> modified_keys_list.hash();
    out.state_db_hash = global_context.state_db.hash();
    out.contract_db_hash = global_context.contract_db.hash();
    return out;
}

std::optional<BlockHeader>
VirtualMachine::try_exec_tx_block(std::vector<SignedTransaction> const& txs)
{
    assert_initialized();

    auto res = validate_tx_block(txs);

    // TBB joins all the threads it uses

    if (!res) {
        phase_undo_block(global_context, *current_block_context);
        advance_block_number();
        return std::nullopt;
    } 

    phase_finish_block(global_context, *current_block_context);

    // now time for hashing a block header
    BlockHeader out = make_block_header();

    prev_block_hash = hash_xdr(out);

    advance_block_number();
    return out;
}

std::pair<BlockHeader, Block>
VirtualMachine::propose_tx_block(AssemblyLimits& limits, uint64_t max_time_ms, uint32_t n_threads)
{
    constexpr static uint32_t max_worker_threads = 20;

    ThreadlocalContextStore::get_rate_limiter().prep_for_notify();
    StaticAssemblyWorkerCache::start_assembly_threads(mempool, global_context, *current_block_context, limits, max_worker_threads);
    ThreadlocalContextStore::enable_rpcs();
    ThreadlocalContextStore::get_rate_limiter().start_threads(n_threads);

    using namespace std::chrono_literals;

    limits.wait_for(max_time_ms * 1ms);

    ThreadlocalContextStore::get_rate_limiter().stop_threads();
    ThreadlocalContextStore::stop_rpcs();

    //ThreadlocalContextStore::join_all_threads();
    std::printf("done stop\n");
    StaticAssemblyWorkerCache::wait_for_stop_assembly_threads();
    std::printf("done join assembly threads\n");

    phase_finish_block(global_context, *current_block_context);
    std::printf("done finish block\n");

    BlockHeader out = make_block_header();
    std::printf("done make header\n");

    prev_block_hash = hash_xdr(out);

    auto block_out = current_block_context -> tx_set.serialize_block();

    advance_block_number();
    std::printf("done proposal\n");
    return {out, block_out};
}

VirtualMachine::~VirtualMachine()
{
    ThreadlocalContextStore::get_rate_limiter().stop_threads();
    ThreadlocalContextStore::stop_rpcs();
    StaticAssemblyWorkerCache::wait_for_stop_assembly_threads();
    // execution context has dangling reference to GlobalContext without this
    ThreadlocalContextStore::clear_entire_context();
}

} // namespace scs
