#include "mutator/batch_filter.h"

#include "state_db/delta_vec.h"

#include "xdr/storage_delta.h"

#include "mtt/utils/time.h"

#include "state_db/delta_batch_value.h"
#include "tx_block/tx_block.h"

#include "object/make_delta.h"

#include "crypto/hash.h"

using namespace scs;
using namespace utils;

auto make_priority = [] (Hash const& h) -> DeltaPriority
{
	DeltaPriority p;
	p.tx_hash = h;
	return p;
};

void run_serial_filter(size_t experiment_size)
{
	auto ts = init_time_measurement();

	DeltaBatchValue dbv;
	TxBlock txs;

	dbv.vectors.push_back(std::make_unique<DeltaVector>());

	auto make_tx = [&](uint64_t idx) -> Hash
	{
		// just need different hashes
		Transaction tx;
		tx.gas_limit = idx;
		return txs.insert_tx(tx);
	};

	auto& vec = *dbv.vectors.back();

	std::vector<Hash> hashes;

	for (size_t i = 0; i < experiment_size; i++)
	{
		auto h = make_tx(i);

		hashes.push_back(h);

		//half are valid
		vec.add_delta(make_nonnegative_int64_set_add(experiment_size, -2), make_priority(h));
	}

	dbv.context = std::make_unique<DeltaBatchValueContext>(vec.get_typeclass_vote());

	std::printf("init runtime: %lf\n", measure_time(ts));

	filter_serial(dbv, txs);
	std::printf("filter runtime: %lf\n", measure_time(ts));

	size_t valid_count = 0;
	for (auto const& h : hashes)
	{
		if (txs.is_valid(TransactionFailurePoint::FINAL, h))
		{
			valid_count ++;
		}
	}
	std::printf("num valid: %lu\n", valid_count);
}

void run_deltavec_merge(size_t experiment_size, size_t num_sub_batches)
{
	DeltaBatchValue dbv;

	auto ts = init_time_measurement();

	for (size_t batch = 0; batch < num_sub_batches; batch++)
	{

		dbv.vectors.push_back(std::make_unique<DeltaVector>());
		auto& vec = *dbv.vectors.back();

		for (size_t i = 0; i < experiment_size / num_sub_batches; i++)
		{
			auto h = hash_xdr<uint64_t>(i + batch * (experiment_size / num_sub_batches));

			vec.add_delta(make_nonnegative_int64_set_add(experiment_size, -2), make_priority(h));
		}
	}
	std::printf("make vecs: %lf\n", measure_time(ts));


	dbv.context = std::make_unique<DeltaBatchValueContext>(dbv.vectors[0]->get_typeclass_vote());
	for (auto& dvs : dbv.vectors)
	{
		dbv.context->dv_all.add(std::move(*dvs));
	}

	std::printf("merge time: %lf\n", measure_time(ts));
}

int main(int argc, const char** argv)
{
	if (argc < 2)
	{
		std::printf("./filter_perf <experiment number> <param1> <param2>\n");
		std::printf("0: serial batch filter\n");
		return -1;
	}

	auto require_argc = [&] (int expect)
	{
		if (argc != expect)
		{
			throw std::runtime_error("invalid number of args");
		}
	};

	size_t experiment_number = std::stoi(argv[1]);

	switch(experiment_number)
	{
		case 0:
			require_argc(3);
			run_serial_filter(std::stoi(argv[2]));
			break;
		case 1:
			require_argc(4);
			run_deltavec_merge(std::stoi(argv[2]), std::stoi(argv[3]));
			break;
	}
}
