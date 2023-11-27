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

#include "vm/base_vm.h"

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

#define VM(ret) template<typename GlobalContext_t, typename BlockContext_t> ret BaseVirtualMachine<GlobalContext_t, BlockContext_t>

template class BaseVirtualMachine<GlobalContext, BlockContext>;
template class BaseVirtualMachine<GroundhogGlobalContext, GroundhogBlockContext>;

VM(void)::init_default_genesis()
{
    install_genesis_contracts(global_context.contract_db);
    current_block_context = std::make_unique<BlockContext_t>(0);
}

VM(void)::assert_initialized() const
{
    if (!current_block_context)
    {
        throw std::runtime_error("uninitialized");
    }
}

VM(bool)::validate_tx_block(Block const& txs)
{
    std::atomic<bool> found_error = false;

    ValidateReduce reduce(
        found_error, global_context, *current_block_context, txs);

    tbb::parallel_reduce(tbb::blocked_range<size_t>(0, txs.transactions.size()), reduce);

    return !found_error;
}


VM(void)::advance_block_number()
{
    current_block_context -> reset_context(
        current_block_context -> block_number + 1);
}

VM(BlockHeader)::make_block_header() 
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

VM(std::optional<BlockHeader>)::try_exec_tx_block(Block const& block)
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

VM(uint64_t)::get_current_block_number() const
{
    return current_block_context -> block_number;
}

VM()::~BaseVirtualMachine()
{
    ThreadlocalContextStore::get_rate_limiter().stop_threads();
    ThreadlocalContextStore::stop_rpcs();
    StaticAssemblyWorkerCache<GlobalContext_t, BlockContext_t>::wait_for_stop_assembly_threads();
    // execution context used to have dangling reference to GlobalContext without this
    ThreadlocalContextStore::clear_entire_context<typename BlockContext::tx_context_t>();
}

} // namespace scs
