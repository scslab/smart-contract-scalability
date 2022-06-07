#include "state_db/delta_batch.h"
#include "state_db/delta_vec.h"
#include "state_db/serial_delta_batch.h"

#include "state_db/state_db.h"

#include "tx_block/tx_block.h"

namespace scs
{

void 
DeltaBatch::merge_in_serial_batches(batch_array_t&& batches)
{

	for (auto& b : batches)
	{
		if (!b) continue;

		auto& m = b->get_delta_map();

		for (auto& [k, v] : m)
		{
			auto it = deltas.emplace(k, value_t());
			it.first->second.vec.add(std::move(v.vec));
		}
	}
}

void 
DeltaBatch::filter_invalid_deltas(TxBlock& txs) {
	if (!populated)
	{
		throw std::runtime_error("cannot filter before populating with db values");
	}
	for (auto& [_, v] : deltas)
	{
		v.mutator.filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(v.vec, txs);
	}
	filtered = true;
}

void 
DeltaBatch::apply_valid_deltas(TxBlock const& txs)
{
	if (!filtered)
	{
		throw std::runtime_error("cannot apply before filtering");
	}
	for (auto& [_, v] : deltas)
	{
		v.mutator.apply_valid_deltas(v.vec, txs);
	}
	applied = true;
}

} /* scs */
