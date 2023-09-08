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

#include "vm/vm.h"

#include <tbb/parallel_reduce.h>

#include <atomic>

#include "crypto/hash.h"
#include "phase/phases.h"
#include "threadlocal/threadlocal_context.h"
#include "transaction_context/transaction_context.h"
#include "transaction_context/execution_context.h"

#include "vm/genesis.h"

#include "block_assembly/limits.h"
#include "block_assembly/assembly_worker.h"

#include "groundhog/types.h"

#include <utils/time.h>

namespace scs {

void
VirtualMachine::init_default_genesis()
{
    install_genesis_contracts(global_context.contract_db);
    current_block_context = std::make_unique<GroundhogBlockContext>(0);
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
    GroundhogBlockContext& block_context;
    Block const& txs;

    void operator()(const tbb::blocked_range<std::size_t> r)
    {
        if (found_error)
            return;

        ThreadlocalTransactionContextStore<GroundhogTxContext>::make_ctx();

        // TBB docs suggest this type of pattern (use local var until end)
        //  optimizes better.
        bool local_found_error = false;

        for (size_t i = r.begin(); i < r.end(); i++) {

            auto& exec_ctx = ThreadlocalTransactionContextStore<GroundhogTxContext>::get_exec_ctx();

            auto const& txset_entry = txs.transactions[i];
            auto const& tx = txset_entry.tx;

            auto hash = hash_xdr(tx);

            for (size_t j = 0; j < txset_entry.nondeterministic_results.size(); j++)
            {
                auto status = exec_ctx.execute(hash, tx, global_context, block_context, txset_entry.nondeterministic_results[j]);

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
                   GlobalContext& global_context,
                   GroundhogBlockContext& block_context,
                   Block const& txs)
        : found_error(found_error)
        , global_context(global_context)
        , block_context(block_context)
        , txs(txs)
    {}
};

bool
VirtualMachine::validate_tx_block(Block const& txs)
{
    std::atomic<bool> found_error = false;

    ValidateReduce reduce(
        found_error, global_context, *current_block_context, txs);

    tbb::parallel_reduce(tbb::blocked_range<size_t>(0, txs.transactions.size()), reduce);

    return !found_error;
}


void
VirtualMachine::advance_block_number()
{
    current_block_context -> reset_context(
        current_block_context -> block_number + 1);
}

BlockHeader
VirtualMachine::make_block_header() 
{
    BlockHeader out;

    tbb::task_group g1, g2;

    out.block_number = current_block_context -> block_number;
    out.prev_header_hash = prev_block_hash;
    g1.run([&] () {
   	 out.tx_set_hash = current_block_context -> tx_set.hash();
	 });
    
    g2.run([&] () {
		    out.modified_keys_hash = current_block_context -> modified_keys_list.hash();
		    });
    out.state_db_hash = global_context.state_db.hash();
    out.contract_db_hash = global_context.contract_db.hash();
    g1.wait();
    g2.wait();
    return out;
}

std::optional<BlockHeader>
VirtualMachine::try_exec_tx_block(Block const& block)
{
    assert_initialized();

    auto res = validate_tx_block(block);

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

BlockHeader
VirtualMachine::propose_tx_block(AssemblyLimits& limits, uint64_t max_time_ms, uint32_t n_threads, Block& block_out)
{
	auto ts = utils::init_time_measurement();
    ThreadlocalContextStore::get_rate_limiter().prep_for_notify();
    ThreadlocalContextStore::enable_rpcs();
    StaticAssemblyWorkerCache::start_assembly_threads(mempool, global_context, *current_block_context, limits, n_threads);
    std::printf("start assembly threads time %lf\n", utils::measure_time(ts));
    ThreadlocalContextStore::get_rate_limiter().start_threads(n_threads);

    using namespace std::chrono_literals;
	
    std::printf("rate limiter start threads time %lf\n", utils::measure_time(ts));

    limits.wait_for(max_time_ms * 1ms);

    std::printf("wait time: %lf\n", utils::measure_time(ts));

    ThreadlocalContextStore::get_rate_limiter().stop_threads();
    ThreadlocalContextStore::stop_rpcs();

    std::printf("stop time %lf\n", utils::measure_time(ts));
    StaticAssemblyWorkerCache::wait_for_stop_assembly_threads();
    std::printf("done join assembly threads %lf\n", utils::measure_time(ts));

    //phase_finish_block(global_context, *current_block_context);
    //std::printf("done finish block %lf\n", utils::measure_time(ts))
    BlockHeader out;
    //auto block_out = std::make_unique<Block>();

    tbb::task_group txset;
    tbb::task_group modlog_merge;
    modlog_merge.run([&]() {
	current_block_context -> modified_keys_list.merge_logs();
	});
    txset.run([&] () {
	current_block_context -> tx_set.finalize();
	out.tx_set_hash = current_block_context -> tx_set.hash();
	current_block_context -> tx_set.serialize_block(block_out);
	});
		    
//	block_structures->tx_set.finalize();
   
//	current_block_context->modified_keys_list.merge_logs();
    modlog_merge.wait();
	std::printf("keys merge logs time %lf\n", utils::measure_time(ts));
	tbb::task_group rest_of_hash;
    rest_of_hash.run([&] ()
		    {
			out.block_number = current_block_context -> block_number;
   			 out.prev_header_hash = prev_block_hash;
    			out.modified_keys_hash = current_block_context -> modified_keys_list.hash();
		    });

	global_context.contract_db.commit();
	global_context.state_db.commit_modifications(current_block_context->modified_keys_list);
	ThreadlocalContextStore::post_block_clear<GroundhogTxContext>();

	std::printf("done commit mods %lf\n", utils::measure_time(ts));
	out.state_db_hash = global_context.state_db.hash();
	out.contract_db_hash = global_context.contract_db.hash();
    
    //BlockHeader out = make_block_header();
    //std::printf("done make header %lf\n", utils::measure_time(ts));

    //prev_block_hash = hash_xdr(out);

    //auto block_out = current_block_context -> tx_set.serialize_block();
	std::printf("done statedb hash %lf\n", utils::measure_time(ts));
    txset.wait();
    rest_of_hash.wait();
    std::printf("final wait time %lf\n", utils::measure_time(ts));
    advance_block_number();
    std::printf("done proposal %lf\n", utils::measure_time(ts));
    return out;
}

uint64_t 
VirtualMachine::get_current_block_number() const
{
    return current_block_context -> block_number;
}

VirtualMachine::~VirtualMachine()
{
    ThreadlocalContextStore::get_rate_limiter().stop_threads();
    ThreadlocalContextStore::stop_rpcs();
    StaticAssemblyWorkerCache::wait_for_stop_assembly_threads();
    // execution context used to have dangling reference to GlobalContext without this
    ThreadlocalContextStore::clear_entire_context<GroundhogTxContext>();
}

} // namespace scs
