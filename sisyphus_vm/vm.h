#pragma once

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

#include "transaction_context/global_context.h"

#include <memory>
#include <vector>

#include "xdr/transaction.h"
#include "xdr/block.h"

#include <utils/non_movable.h>

#include "mempool/mempool.h"

#include "state_db/async_keys_to_disk.h"
#include "block_assembly/assembly_worker.h"

namespace scs {

class AssemblyLimits;

/**
 * Block policy:
 * Header is <hashes of data structures, prev hash, etc>
 * We reject a block if prev_hash doesn't match our hash,
 * but if we execute a block and find no errors
 * but find a hash mismatch at the end,
 * we say that's ok, and move on (presumably rejecting subsequent
 * blocks from that proposer).
 * 
 * Sisyphus version
 */
class SisyphusVirtualMachine : public utils::NonMovableOrCopyable
{
    SisyphusGlobalContext global_context;
    std::unique_ptr<SisyphusBlockContext> current_block_context;
    Mempool mempool;

    AssemblyWorkerCache<SisyphusGlobalContext, SisyphusBlockContext> worker_cache;
    Hash prev_block_hash;

    AsyncKeysToDisk keys_persist;

    void assert_initialized() const;

    bool validate_tx_block(Block const& txs);

    void advance_block_number();

    BlockHeader make_block_header();

  public:
    SisyphusVirtualMachine()
	    :global_context()
	    , current_block_context()
	     , mempool()
	     , worker_cache(mempool, global_context)
	{}
    void init_default_genesis();

    std::optional<BlockHeader>
    try_exec_tx_block(Block const& txs);

    Mempool& get_mempool() {
      return mempool;
    }

    BlockHeader propose_tx_block(AssemblyLimits& limits, uint64_t max_time_ms, uint32_t n_threads, Block& out, ModIndexLog& out_modlog,
      std::unique_ptr<SisyphusBlockContext>* extract_block_context = nullptr);

    uint64_t get_current_block_number() const;

    const auto& get_global_context() const {
      return global_context;  
    }

    ~SisyphusVirtualMachine();
};

} // namespace scs
