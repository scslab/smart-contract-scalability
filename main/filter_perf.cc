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

	auto make_priority = [] (Hash const& h) -> DeltaPriority
	{
		DeltaPriority p;
		p.tx_hash = h;
		return p;
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

	dbv.context = std::make_optional<DeltaBatchValueContext>(vec.get_typeclass_vote());

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

int main(int argc, const char** argv)
{
	if (argc != 3)
	{
		std::printf("./filter_perf <experiment number> <experiment size>\n");
		std::printf("0: serial batch filter\n");
	}

	size_t experiment_number = std::stoi(argv[1]);
	size_t experiment_size = std::stoi(argv[2]);

	switch(experiment_number)
	{
		case 0:
			run_serial_filter(experiment_size);
			break;
	}
}
