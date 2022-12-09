#include "block_assembly/assembly_worker.h"

#include "block_assembly/limits.h"

#include "transaction_context/global_context.h"

#include "mempool/mempool.h"

#include "threadlocal/threadlocal_context.h"

#include "crypto/hash.h"

namespace scs {

void
AssemblyWorker::run()
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
        if (!reservation)
        {
            limits.notify_done();
            return;
        }

        ThreadlocalContextStore::make_ctx(global_context);
        auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

        auto result = exec_ctx.execute(hash_xdr(*tx), *tx, block_context);
        if (result == TransactionStatus::SUCCESS)
        {
            reservation->commit();
        }
    }
}

void
AsyncAssemblyWorker::run()
{
    while (true) {
        {
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
        }
        cv.notify_all();
    }
}

} // namespace scs
