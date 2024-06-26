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

#include "sisyphus_vm/vm.h"

#include <tbb/parallel_reduce.h>

#include <atomic>

#include "crypto/hash.h"
#include "phase/phases.h"
#include "threadlocal/threadlocal_context.h"
#include "transaction_context/transaction_context.h"
#include "transaction_context/execution_context.h"

#include "vm/genesis.h"
#include "vm/validate_reduce.h"

#include "block_assembly/limits.h"
#include "block_assembly/assembly_worker.h"

#include <utils/time.h>

namespace scs {

/*
void
SisyphusVirtualMachine::init_default_genesis()
{
    install_genesis_contracts(global_context.contract_db);
    current_block_context = std::make_unique<SisyphusBlockContext>(0);
}

void
SisyphusVirtualMachine::assert_initialized() const
{
    if (!current_block_context)
    {
        throw std::runtime_error("uninitialized");
    }
}

#if 0

struct SisyphusValidateReduce
{
    std::atomic<bool>& found_error;
    SisyphusGlobalContext& global_context;
    SisyphusBlockContext& block_context;
    Block const& txs;

    void operator()(const tbb::blocked_range<std::size_t> r)
    {
        if (found_error)
            return;

        ThreadlocalTransactionContextStore<SisyphusTxContext>::make_ctx();

        // TBB docs suggest this type of pattern (use local var until end)
        //  optimizes better.
        bool local_found_error = false;

        for (size_t i = r.begin(); i < r.end(); i++) {

            auto& exec_ctx = ThreadlocalTransactionContextStore<SisyphusTxContext>::get_exec_ctx();

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
    SisyphusValidateReduce(SisyphusValidateReduce& x, tbb::split)
        : found_error(x.found_error)
        , global_context(x.global_context)
        , block_context(x.block_context)
        , txs(x.txs){};

    void join(SisyphusValidateReduce& other) {}

    SisyphusValidateReduce(std::atomic<bool>& found_error,
                   SisyphusGlobalContext& global_context,
                   SisyphusBlockContext& block_context,
                   Block const& txs)
        : found_error(found_error)
        , global_context(global_context)
        , block_context(block_context)
        , txs(txs)
    {}
};

#endif

bool
SisyphusVirtualMachine::validate_tx_block(Block const& txs)
{
    std::atomic<bool> found_error = false;

    ValidateReduce reduce(
        found_error, global_context, *current_block_context, txs);

    tbb::parallel_reduce(tbb::blocked_range<size_t>(0, txs.transactions.size()), reduce);

    return !found_error;
}


void
SisyphusVirtualMachine::advance_block_number()
{
    current_block_context -> reset_context(
        current_block_context -> block_number + 1);
}

BlockHeader
SisyphusVirtualMachine::make_block_header() 
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
} */

std::optional<BlockHeader>
SisyphusVirtualMachine::try_exec_tx_block(Block const& block)
{
    // BaseVirtualMachine::try_exec_tx_block
    /*assert_initialized();

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
    return out; */

    auto out = BaseVirtualMachine::try_exec_tx_block(block);

    if (!out) {

        // It should be the case (and it is for our current implementation of the memcache trie)
        // that a no-op block doesn't log anything to storage.
        // (This is true because we don't call compute_hash() unless we commit to the block).
        // A more complicated eviction policy might push things to storage.
        // It's not wrong if that happens, but we just don't want the in-memory buffer to 
        // build up too much.
        global_context.state_db.set_timestamp(current_block_context -> block_number);
        
        return std::nullopt;
    }

/*

    assert_initialized();

    auto res = validate_tx_block(block);

    // TBB joins all the threads it uses

    if (!res) {
        phase_undo_block(global_context, *current_block_context);
        advance_block_number();

        // It should be the case (and it is for our current implementation of the memcache trie)
        // that a no-op block doesn't log anything to storage.
        // (This is true because we don't call compute_hash() unless we commit to the block).
        // A more complicated eviction policy might push things to storage.
        // It's not wrong if that happens, but we just don't want the in-memory buffer to 
        // build up too much.
        global_context.state_db.set_timestamp(current_block_context -> block_number);
        
        return std::nullopt;
    } 

    phase_finish_block(global_context, *current_block_context);

    // now time for hashing a block header
    BlockHeader out = make_block_header();

    prev_block_hash = hash_xdr(out);

    advance_block_number();
    */

    auto ts = utils::init_time_measurement();
    global_context.state_db.log_keys(keys_persist);
    global_context.state_db.set_timestamp(current_block_context -> block_number);
    std::printf("in try_exec wait for statedb log time %lf\n", utils::measure_time(ts));
    return out;
}

BlockHeader
SisyphusVirtualMachine::propose_tx_block(AssemblyLimits& limits, uint64_t max_time_ms, uint32_t n_threads, Block& block_out, ModIndexLog& out_modlog, 
    std::unique_ptr<SisyphusBlockContext>* extract_block_context)
{
	auto ts = utils::init_time_measurement();
    ThreadlocalContextStore::get_rate_limiter().prep_for_notify();
    ThreadlocalContextStore::enable_rpcs();
    ThreadlocalContextStore::get_rate_limiter().start_threads(n_threads);

    worker_cache.start_assembly_threads(
        current_block_context.get(), &limits, n_threads);
    std::printf("start assembly threads time %lf\n", utils::measure_time(ts));

    using namespace std::chrono_literals;
	
    std::printf("rate limiter start threads time %lf\n", utils::measure_time(ts));

    limits.wait_for(max_time_ms * 1ms);

    std::printf("wait time: %lf\n", utils::measure_time(ts));

    ThreadlocalContextStore::get_rate_limiter().stop_threads();
    ThreadlocalContextStore::stop_rpcs();

    std::printf("stop time %lf\n", utils::measure_time(ts));
    worker_cache.wait_for_stop_assembly_threads();
    std::printf("done join assembly threads %lf\n", utils::measure_time(ts));

    BlockHeader out;

    tbb::task_group txset;

    txset.run(
        [&] () {
        	current_block_context -> tx_set.finalize();
        	out.tx_set_hash = current_block_context -> tx_set.hash();
        	current_block_context -> tx_set.serialize_block(block_out);
    	});
		    
	std::printf("keys merge logs time %lf\n", utils::measure_time(ts));
	tbb::task_group rest_of_hash;
    rest_of_hash.run([&] ()
		    {
			     out.block_number = current_block_context -> block_number;
   			      out.prev_header_hash = prev_block_hash;
    			 out.modified_keys_hash = current_block_context -> modified_keys_list.hash();
                 current_block_context -> modified_keys_list.save_modifications(out_modlog);
		    });

	global_context.contract_db.commit(get_current_block_number());
	global_context.state_db.commit_modifications(current_block_context->modified_keys_list);
	ThreadlocalContextStore::post_block_clear();

	std::printf("done commit mods %lf\n", utils::measure_time(ts));
	out.state_db_hash = global_context.state_db.hash();
	out.contract_db_hash = global_context.contract_db.hash();
    
	std::printf("done statedb hash %lf\n", utils::measure_time(ts));
    txset.wait();
    rest_of_hash.wait();
    std::printf("final wait time %lf\n", utils::measure_time(ts));
    if (extract_block_context != nullptr) {
        uint64_t current_block_number = get_current_block_number();
        (*extract_block_context).reset(current_block_context.release());
        current_block_context = std::make_unique<SisyphusBlockContext>(current_block_number + 1);
    } 
    else
    {
        advance_block_number();
    }
    std::printf("done proposal %lf\n", utils::measure_time(ts));

    global_context.state_db.log_keys(keys_persist);
    global_context.state_db.set_timestamp(current_block_context -> block_number);
    std::printf("wait for statedb log time %lf\n", utils::measure_time(ts));
    return out;
}

/*
uint64_t 
SisyphusVirtualMachine::get_current_block_number() const
{
    return current_block_context -> block_number;
}

SisyphusVirtualMachine::~SisyphusVirtualMachine()
{
    ThreadlocalContextStore::get_rate_limiter().stop_threads();
    ThreadlocalContextStore::stop_rpcs();
    // execution context used to have dangling reference to GlobalContext without this
    ThreadlocalContextStore::clear_entire_context<SisyphusTxContext>();
}
*/

} // namespace scs
