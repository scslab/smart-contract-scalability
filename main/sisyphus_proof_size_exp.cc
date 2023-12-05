#include "experiments/payment_experiment.h"

#include <utils/time.h>

#include "vm/vm.h"

#include <cstdint>

#include "block_assembly/limits.h"

#include "persistence/persist_xdr.h"

#include <tbb/global_control.h>

using namespace scs;

AsyncPersistXDR<Block> block_persist("block_logs/");

AsyncPersistXDR<ModIndexLog> log_persist("modlog_logs/");

void
run_experiment(uint32_t num_accounts,
               uint32_t batch_size,
               uint32_t num_threads,
               uint32_t num_blocks,
	       uint16_t size_boost)
{
    PaymentExperiment e(num_accounts, size_boost);

    auto vm = e.prepare_sisyphus_vm();

    if (!vm) {
        throw std::runtime_error("failed to initialize virtual machine!");
    }

    auto& mp = vm->get_mempool();

    const uint32_t tx_batch_buffer
        = 10 + (num_blocks) * (num_threads) / (batch_size);

    for (size_t i = 0; i < num_blocks + tx_batch_buffer; i++) {
        if (mp.add_txs(e.gen_transaction_batch(batch_size, i + vm -> get_current_block_number())) != batch_size) {
            throw std::runtime_error("failed to add txs to batch!");
        }
    }

    auto ts = utils::init_time_measurement();

    Block block_buffer;
    ModIndexLog log_buffer;

    block_persist.clear_folder();

    log_persist.clear_folder();

    for (size_t i = 0; i < num_blocks; i++) {
        AssemblyLimits limits(batch_size, INT64_MAX);

        auto ts_local = utils::init_time_measurement();

        uint32_t blk_number = vm -> get_current_block_number();

        std::unique_ptr<SisyphusBlockContext> blk_context;

    	auto header
                = vm->propose_tx_block(limits, 100'000, num_threads, block_buffer, log_buffer, &blk_context);

        uint64_t blk_size = block_buffer.transactions.size();
        double duration = utils::measure_time(ts);

        block_persist.log(block_buffer, blk_number);
        log_persist.log(log_buffer, blk_number);

        auto const& sdb = vm -> get_global_context().state_db;
        auto const& index = blk_context -> modified_keys_list;

        std::map<DeltaType, std::vector<uint32_t>> stats;

        for (auto const& i : log_buffer)
        {
            auto key = make_index_key(i.addr, i.delta, i.tx_hash);
            auto pf = index.get_keys().make_proof(key, key.len());
            stats[i.delta.type()].push_back(pf.serialize().size());
        }

        for (auto it = stats.begin(); it != stats.end(); it++)
        {
            uint32_t max = 0, min = UINT32_MAX;
            uint64_t sum = 0;

            for (auto const& val : it -> second)
            {
                sum += val;
                max = std::max(max, val);
                min = std::min(min, val);
            }

            std::printf("nacc %lu batch %lu nthread %lu stat %u min %u max %u avg %lf\n", 
                num_accounts,
                batch_size,
                num_threads,
                it -> first, min, max, ((double)sum) / it -> second.size());
        }

        std::printf("duration: %lf size %lu rate %lf remaining_mempool %lu \n",
                    duration,
                    blk_size,
                    blk_size / duration,
                    mp.available_size());
        std::fprintf(
            stderr,
            "duration %lf rate %lf nacc %lu batch %lu nthread %lu i %lu\n",
            duration,
            blk_size / duration,
            num_accounts,
            batch_size,
            num_threads,
            i);

        if (blk_size != batch_size) {
        	std::printf("%lu != %lu\n", blk_size, batch_size);
	    	throw std::runtime_error("batch size mismatch!");
        }
    }
    log_persist.wait_for_async_task();
    block_persist.wait_for_async_task();
}

int
main(int argc, const char** argv)
{
    std::vector<uint32_t> accts = { 2, 10, 100, 1000, 10'000 };
    std::vector<uint32_t> batches = {100, 1'000,  10'000, 100'000 };
    std::vector<uint32_t> nthreads = { 64 };
    std::vector<uint32_t> big_accts = {  100'000,  1'000'000 , 10'000'000  };

    bool short_stuff = false;
	bool long_stuff = true;
    if (short_stuff)
    {

    for (auto acct : accts) {
        for (auto batch : batches) {
            for (auto nthread : nthreads) {
                std::printf("start %lu %lu %lu\n", acct, batch, nthread);
                uint32_t trials = 15;
                // 10 trials, 5 warmup
                run_experiment(acct, batch, nthread, trials, UINT16_MAX);
            }
        }
    }
    }
    if (long_stuff)
    {
        for (auto acct : big_accts) {
            for (auto nthread : nthreads) {
                uint32_t batch = 100'000;
                std::printf("start %lu %lu %lu\n", acct, batch, nthread);
                uint32_t trials = 15;
                // 10 trials, 5 warmup
    	        uint16_t boost = 0;
                run_experiment(acct, batch, nthread, trials, boost);
            }
        }
    }
}
