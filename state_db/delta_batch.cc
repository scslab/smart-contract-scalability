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
	if (populated)
	{
		throw std::runtime_error("add batches after populating");
	}
	for (auto& b : batches)
	{
		if (!b) continue;

		auto& m = b->get_delta_map();

		for (auto& [k, v] : m)
		{
			auto it = deltas.find(k);

			auto tc = (*v.vectors.begin())->get_typeclass_vote();
			if (it == deltas.end())
			{
				it = deltas.emplace(k, value_t(tc)).first;
			}
			//it->second.vec.add(std::move(v.vec));
			if (it -> second.context->typeclass.is_lower_rank_than(tc))
			{
				it->second.context->typeclass = tc;
			}
			auto& main_vec = it->second.vectors;

			main_vec.insert(main_vec.end(), std::make_move_iterator(v.vectors.begin()), std::make_move_iterator(v.vectors.end()));
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
		for (auto& dvs : v.vectors)
		{
			v.context->dv_all.add(std::move(*dvs));
		}
		// TODO switch away from singlethreaded mutator
		v.context -> mutator.filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(v.context -> dv_all, txs);
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
		v.context -> mutator.apply_valid_deltas(v.context -> dv_all, txs);
	}
	applied = true;
}

} /* scs */
