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
#include "block_assembly/assembly_worker.h"

namespace scs {

class AssemblyLimits;

template<typename GlobalContext_t, typename BlockContext_t>
class BaseVirtualMachine : public utils::NonMovableOrCopyable
{
  protected:
    GlobalContext_t global_context;
    std::unique_ptr<BlockContext_t> current_block_context;
    Mempool mempool;

    AssemblyWorkerCache<GlobalContext_t, BlockContext_t> worker_cache;

    Hash prev_block_hash;

    using TransactionContext_t = typename BlockContext_t::tx_context_t;

  private:
    utils::ThreadlocalCache<ExecutionContext<TransactionContext_t>, TLCACHE_SIZE> executors;

  protected:

    void assert_initialized() const;

    bool validate_tx_block(Block const& txs);

    void advance_block_number();

    BlockHeader make_block_header();

  public:
    BaseVirtualMachine()
	    : global_context()
	    , current_block_context()
	      , mempool()
	      , worker_cache(mempool, global_context)
	      , prev_block_hash()
        , executors()
	{}
    
    void init_default_genesis();

    std::optional<BlockHeader>
    try_exec_tx_block(Block const& txs);

    Mempool& get_mempool() {
      return mempool;
    }

    uint64_t get_current_block_number() const;

    ~BaseVirtualMachine();
};

} // namespace scs
