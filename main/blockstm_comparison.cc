#include "experiments/payment_experiment.h"

#include <utils/time.h>

#include "vm/vm.h"

#include <cstdint>

#include "block_assembly/limits.h"

#include <tbb/global_control.h>

using namespace scs;

std::vector<double>
run_experiment(uint32_t num_accounts,
               uint32_t batch_size,
               uint32_t num_threads,
               uint32_t num_blocks,
	       uint16_t size_boost,
	       bool use_native_sig,
	       wasm_api::SupportedWasmEngine engine)
{
    std::printf("using wasm engine %s native_sig %u\n", wasm_api::engine_to_string(engine).c_str(), use_native_sig);
    PaymentExperiment e(num_accounts, use_native_sig, engine, size_boost);

    auto vm = e.prepare_vm();

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

    for (size_t i = 0; i < num_blocks; i++) {
        AssemblyLimits limits(batch_size, INT64_MAX);

        auto ts_local = utils::init_time_measurement();

    	auto header
                = vm->propose_tx_block(limits, 300'000, num_threads, block_buffer);
        std::printf("local measurement %lf\n", utils::measure_time(ts_local));

        uint64_t blk_size = block_buffer.transactions.size();
        double duration = utils::measure_time(ts);

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
    return out;
}

int
main(int argc, const char** argv)
{
    std::vector<uint32_t> accts = { 2, 10, 100, 1000, 10'000 };
    std::vector<uint32_t> batches = { 100'000 };
    std::vector<uint32_t> nthreads = { 1, 8, 16, 32, 64, 96, 128, 160, 192 };
    std::vector<uint32_t> big_accts = { 100'000,  1'000'000  };
    std::vector<bool> sigs = { true, false};
    std::vector<wasm_api::SupportedWasmEngine> engines = {
	    wasm_api::SupportedWasmEngine::WASMI,
	    wasm_api::SupportedWasmEngine::WASM3,
	    wasm_api::SupportedWasmEngine::WASMTIME};

    struct exp_res
    {
        uint32_t acct;
        uint32_t batch;
        uint32_t nthread;
        double avg;
	bool native_sig;
	wasm_api::SupportedWasmEngine engine;

        void print()
        {
            std::printf("result: acct %lu batch %lu nthread %lu avg %lf native_sig %u engine %s\n",
                        acct,
                        batch,
                        nthread,
                        avg,
			native_sig,
			wasm_api::engine_to_string(engine).c_str());
        }
    };

    std::printf("Groundhog VM No Persistence (original impl)\n");

    std::vector<exp_res> overall_results;

    const bool short_stuff = false;
	const bool long_stuff = true;
    /*
    if (short_stuff)
    {
        for (auto acct : accts) {
            for (auto batch : batches) {
                for (auto nthread : nthreads) {
                    std::printf("start %lu %lu %lu\n", acct, batch, nthread);
                    uint32_t trials = 25;
                    // 20 trials, 5 warmup
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
    } */
    if (long_stuff)
    {
	for (auto sig : sigs) {
	for (auto engine : engines) {
        for (auto acct : big_accts) {
            for (auto nthread : nthreads) {
                uint32_t batch = 100'000;
                std::printf("start %lu %lu %lu %u %d\n", acct, batch, nthread, uint8_t{sig}, engine);
                uint32_t trials = 25;
                // 20 trials, 5 warmup
                uint16_t boost = 0;
                auto results = run_experiment(acct, batch, nthread, trials, boost, sig, engine);
                double res = 0;
                for (size_t i = 5; i < trials; i++) {
                    res += results[i];
                }
                double avg = res / (trials - 5);

                exp_res r{
                    .acct = acct, .batch = batch, .nthread = nthread, .avg = avg, .native_sig = sig, .engine = engine
                };
                overall_results.push_back(r);
                r.print();
            }
        }
    }
    }
    }

    std::printf("results summary:\n");
    for (auto r : overall_results) {
        r.print();
    }
}
