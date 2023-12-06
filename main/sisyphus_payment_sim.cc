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

std::vector<double>
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
    std::printf("blk num = %lu\n", vm -> get_current_block_number());


    tbb::global_control control(tbb::global_control::max_allowed_parallelism,
                        num_threads);

    std::vector<double> out;

    auto ts = utils::init_time_measurement();

    Block block_buffer;
    ModIndexLog log_buffer;

    block_persist.clear_folder();

    log_persist.clear_folder();

    for (size_t i = 0; i < num_blocks; i++) {
        AssemblyLimits limits(batch_size, INT64_MAX);

        auto ts_local = utils::init_time_measurement();

        uint32_t blk_number = vm -> get_current_block_number();

    	auto header
                = vm->propose_tx_block(limits, 100'000, num_threads, block_buffer, log_buffer);
        std::printf("local measurement %lf\n", utils::measure_time(ts_local));

        uint64_t blk_size = block_buffer.transactions.size();
        double duration = utils::measure_time(ts);

        block_persist.log(block_buffer, blk_number);
        log_persist.log(log_buffer, blk_number);

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
        out.push_back(blk_size / duration);

        if (blk_size != batch_size) {
        	std::printf("%lu != %lu\n", blk_size, batch_size);
	    	throw std::runtime_error("batch size mismatch!");
        }
    }
    log_persist.wait_for_async_task();
    block_persist.wait_for_async_task();
    return out;
}

int
main(int argc, const char** argv)
{
    std::vector<uint32_t> accts = { 2, 10, 100, 1000, 10'000 };
    std::vector<uint32_t> batches = {100, 1'000,  10'000, 100'000 };
    std::vector<uint32_t> nthreads = { /*1, 2, 4, 8, 16,*/ 32, 64, 96 };
    std::vector<uint32_t> big_accts = {  100'000,  1'000'000 , 10'000'000  };
    struct exp_res
    {
        uint32_t acct;
        uint32_t batch;
        uint32_t nthread;
        double avg;

        void print()
        {
            std::printf("result: acct %lu batch %lu nthread %lu avg %lf\n",
                        acct,
                        batch,
                        nthread,
                        avg);
        }
    };

    std::vector<exp_res> overall_results;

    bool short_stuff = false;
	bool long_stuff = true;
    if (short_stuff)
    {

    for (auto acct : accts) {
        for (auto batch : batches) {
            for (auto nthread : nthreads) {
                std::printf("start %lu %lu %lu\n", acct, batch, nthread);
                uint32_t trials = 25;
                // 10 trials, 5 warmup
                auto results = run_experiment(acct, batch, nthread, trials, UINT16_MAX);
                double res = 0;
                for (size_t i = 5; i < trials; i++) {
                    res += results[i];
                }
                double avg = res / (trials - 5);

                exp_res r{
                    .acct = acct, .batch = batch, .nthread = nthread, .avg = avg
                };
                overall_results.push_back(r);
                r.print();
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
            uint32_t trials = 25;
            // 10 trials, 5 warmup
	        uint16_t boost = 0;
            auto results = run_experiment(acct, batch, nthread, trials, boost);
            double res = 0;
            for (size_t i = 5; i < trials; i++) {
                res += results[i];
            }
            double avg = res / (trials - 5);

            exp_res r{
                .acct = acct, .batch = batch, .nthread = nthread, .avg = avg
            };
            overall_results.push_back(r);
            r.print();
        }
    }
    }

    std::printf("results summary:\n");
    for (auto r : overall_results) {
        r.print();
    }
}
