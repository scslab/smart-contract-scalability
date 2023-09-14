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


#include "block_assembly/assembly_worker.h"
#include "block_assembly/limits.h"

#include "transaction_context/global_context.h"
#include "transaction_context/transaction_context.h"
#include "transaction_context/execution_context.h"

#include "mempool/mempool.h"

#include "threadlocal/threadlocal_context.h"

#include "crypto/hash.h"

namespace scs {

template<typename GlobalContext_t, typename BlockContext_t>
void
AssemblyWorker<GlobalContext_t, BlockContext_t>::run()
{
    auto& limiter = ThreadlocalContextStore::get_rate_limiter();

    while (true) {
        bool is_shutdown = limiter.wait_for_opening();

        if (is_shutdown) {
            return;
        }

        auto tx = mempool.get_new_tx();
        if (!tx) {
            limits.notify_done();
            return;
        }

        auto reservation = limits.reserve_tx(*tx);
        if (!reservation) {
            limits.notify_done();
            return;
        }

        ThreadlocalTransactionContextStore<typename BlockContext_t::tx_context_t>::make_ctx();
        auto& exec_ctx = ThreadlocalTransactionContextStore<typename BlockContext_t::tx_context_t>::get_exec_ctx();

        auto result = exec_ctx.execute(hash_xdr(*tx), *tx, global_context, block_context);
        if (result == TransactionStatus::SUCCESS) {
            reservation->commit();
        }
    	else {
    		std::printf("tx failed\n");
    	}
    }
}

template class AssemblyWorker<GlobalContext, GroundhogBlockContext>;
template class AssemblyWorker<SisyphusGlobalContext, SisyphusBlockContext>;

template<typename worker_t>
void
AsyncAssemblyWorker<worker_t>::run()
{
    while (true) {
        std::unique_lock lock(mtx);
        if ((!done_flag) && (!exists_work_to_do())) {
            cv.wait(lock,
                    [this]() { return done_flag || exists_work_to_do(); });
        }
        if (done_flag) {
            return;
        }

        if (!worker) {
            throw std::runtime_error("shouldn't have null worker here");
        }

        worker->run();
        ThreadlocalContextStore::get_rate_limiter().free_one_slot();

        worker = std::nullopt;
        cv.notify_all();
    }
}

template class
AsyncAssemblyWorker<AssemblyWorker<GlobalContext, GroundhogBlockContext>>;

template class
AsyncAssemblyWorker<AssemblyWorker<SisyphusGlobalContext, SisyphusBlockContext>>;

} // namespace scs
