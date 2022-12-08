#include "experiments/payment_experiment.h"

#include <utils/time.h>

#include "vm/vm.h"

#include <cstdint>

#include "block_assembly/limits.h"

using namespace scs;

std::vector<double>
run_experiment(uint32_t num_accounts, uint32_t batch_size, uint32_t num_blocks)
{
	PaymentExperiment e(num_accounts, UINT16_MAX);

	auto vm = e.prepare_vm();

	auto& mp = vm -> get_mempool();

	constexpr static uint32_t tx_batch_buffer = 5;

	for (size_t i = 0; i < num_blocks + tx_batch_buffer; i++)
	{
		if (mp.add_txs(e.gen_transaction_batch(batch_size)) != batch_size)
		{
			throw std::runtime_error("failed to add txs to batch!");
		}
	}

	std::vector<double> out;

	auto ts = utils::init_time_measurement();

	for (size_t i = 0; i < num_blocks; i++)
	{

		AssemblyLimits limits(10, INT64_MAX);

		auto [header, blk] = vm -> propose_tx_block(limits, 1000, 10);

		uint64_t blk_size = blk.transactions.size();
		double duration = utils::measure_time(ts);

		std::printf("duration: %lf size %lu rate %lf\n", duration, blk_size, blk_size/duration);
		out.push_back(blk_size / duration);

		if (blk_size != batch_size)
		{
			throw std::runtime_error("batch size mismatch!");
		}
	}
	return out;
}

int main(int argc, const char** argv)
{
	std::vector<uint32_t> accts = {2, 10, 100};
	std::vector<uint32_t> batches = {10, 100, 1000, 10'000};

	for (auto acct : accts)
	{
		for (auto batch : batches)
		{
			uint32_t trials = 10;
			auto results = run_experiment(acct, batch, trials);
			double res = 0;
			for (auto r : results)
			{
				res += r;
			}
			double avg = res / trials;

			std::printf("result: acct %lu batch %lu avg %lf\n", acct, batch, avg);
		}
	}
}