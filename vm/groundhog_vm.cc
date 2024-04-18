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

#include "vm/groundhog_vm.h"

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


std::optional<BlockHeader>
GroundhogVirtualMachine::try_exec_tx_block(Block const& txs)
{
    auto out = BaseVirtualMachine<GroundhogGlobalContext, GroundhogBlockContext>::try_exec_tx_block(txs);

    if (out)
    {
        global_context.state_db.log_keys(keys_persist);
        global_context.state_db.set_timestamp(current_block_context -> block_number);
    }
    return out;
}


BlockHeader
GroundhogVirtualMachine::propose_tx_block(AssemblyLimits& limits, uint64_t max_time_ms, uint32_t n_threads, Block& block_out)
{
	auto ts = utils::init_time_measurement();
    ThreadlocalContextStore::get_rate_limiter().prep_for_notify();
    ThreadlocalContextStore::enable_rpcs();
    worker_cache.start_assembly_threads(current_block_context.get(), &limits, n_threads);
    std::printf("start assembly threads time %lf\n", utils::measure_time(ts));
    ThreadlocalContextStore::get_rate_limiter().start_threads(n_threads);

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
    tbb::task_group modlog_merge;

	current_block_context -> modified_keys_list.merge_logs();

    txset.run([&] () {
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
    advance_block_number();
    std::printf("done proposal %lf\n", utils::measure_time(ts));

    global_context.state_db.log_keys(keys_persist);
    global_context.state_db.set_timestamp(current_block_context -> block_number);
    std::printf("wait for statedb log time %lf\n", utils::measure_time(ts));
    return out;
}

} // namespace scs
