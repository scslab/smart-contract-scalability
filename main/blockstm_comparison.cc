#include "experiments/payment_experiment.h"

#include <utils/time.h>

#include "vm/vm.h"

#include <cstdint>

#include "block_assembly/limits.h"

#include <tbb/global_control.h>

using namespace scs;

std::vector<double>
run_experiment(uint32_t num_accounts, uint32_t batch_size, uint32_t num_threads, uint32_t num_blocks)
{
	PaymentExperiment e(num_accounts, UINT16_MAX);

	auto vm = e.prepare_vm();

	if (!vm) {
		throw std::runtime_error("failed to initialize virtual machine!");
	}

	auto& mp = vm -> get_mempool();

	const uint32_t tx_batch_buffer = 10 +  (num_blocks) * (num_threads) / (batch_size);

	for (size_t i = 0; i < num_blocks + tx_batch_buffer; i++)
	{
		if (mp.add_txs(e.gen_transaction_batch(batch_size, i)) != batch_size)
		{
			throw std::runtime_error("failed to add txs to batch!");
		}
	}

	tbb::global_control(tbb::global_control::max_allowed_parallelism, num_threads);

	std::vector<double> out;

	auto ts = utils::init_time_measurement();
	
	Block block_buffer;
	//std::vector<std::unique_ptr<Block>> gc;

	for (size_t i = 0; i < num_blocks; i++)
	{
		AssemblyLimits limits(batch_size, INT64_MAX);

		auto ts_local = utils::init_time_measurement();
		auto header = vm -> propose_tx_block(limits, 100'000, num_threads, block_buffer);
		std::printf("local measurement %lf\n", utils::measure_time(ts_local));
			
		uint64_t blk_size = block_buffer.transactions.size();
		double duration = utils::measure_time(ts);

		std::printf("duration: %lf size %lu rate %lf remaining_mempool %lu \n", duration, blk_size, blk_size/duration, mp.available_size());
		out.push_back(blk_size / duration);

		if (blk_size != batch_size)
		{
			throw std::runtime_error("batch size mismatch!");
		}
//		gc.emplace_back(std::move(blk));
	}
	return out;
}

int main(int argc, const char** argv)
{
	std::vector<uint32_t> accts = {10'000 };//2, 10, 100};
	std::vector<uint32_t> batches = {100'000};
	std::vector<uint32_t> nthreads = {16,32,64,96};

	struct exp_res {
		uint32_t acct;
		uint32_t batch;
		uint32_t nthread;
		double avg;

		void print()
		{
			std::printf("result: acct %lu batch %lu nthread %lu avg %lf\n", acct, batch, nthread, avg);
		}
	};

	std::vector<exp_res> overall_results;

	for (auto acct : accts)
	{
		for (auto batch : batches)
		{
			for (auto nthread : nthreads)
			{
				std::printf("start %lu %lu %lu\n", acct, batch, nthread);
				uint32_t trials = 15;
				// 10 trials, 5 warmup
				auto results = run_experiment(acct, batch, nthread, trials);
				double res = 0;
				for (size_t i = 5; i < trials; i++)
				{
					res += results[i];
				}
				double avg = res / (trials - 5);

				exp_res r {
					.acct = acct,
					.batch = batch,
					.nthread = nthread,
					.avg = avg
				};
				overall_results.push_back(r);
				r.print();
			}
		}
	}
	std::printf("results summary:\n");
	for (auto r : overall_results)
	{
		r.print();
	}
}
