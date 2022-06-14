#include "state_db/delta_batch.h"
#include "state_db/delta_vec.h"

#include "state_db/state_db.h"

#include "tx_block/tx_block.h"

#include "transaction_context/threadlocal_context.h"

namespace scs
{

DeltaBatch::~DeltaBatch()
{
	ThreadlocalContextStore::post_block_clear();
}

SerialDeltaBatch 
DeltaBatch::get_serial_subsidiary()
{
	return SerialDeltaBatch(serial_cache.get(deltas));
}


struct DeltaBatchValueMergeFn
{
	static
	void 
	value_merge(
		DeltaBatchValue& original_value, 
		DeltaBatchValue& merge_in_value)
	{

		if (!original_value.context)
		{
			if (original_value.vectors.size() != 1)
			{
				throw std::runtime_error("invalid original_value");
			}
			original_value.context = std::make_unique<DeltaBatchValueContext>(original_value.vectors.back()->get_typeclass_vote());
		}

		if (!merge_in_value.context)
		{
			if (merge_in_value.vectors.size() != 1)
			{
				throw std::runtime_error("invalid merge_in_value");
			}
			auto tc_check = merge_in_value.vectors.back()->get_typeclass_vote();
			if (original_value.context -> typeclass.is_lower_rank_than(tc_check))
			{
				original_value.context -> typeclass = tc_check;
			}
		} else
		{
			auto const& tc_check = merge_in_value.context -> typeclass;
			if (original_value.context -> typeclass.is_lower_rank_than(tc_check))
			{
				original_value.context -> typeclass = tc_check;
			}
		}

		original_value.vectors.insert(original_value.vectors.end(),
			std::make_move_iterator(merge_in_value.vectors.begin()),
			std::make_move_iterator(merge_in_value.vectors.end()));
	}
};

void 
DeltaBatch::merge_in_serial_batches()
{
	if (populated)
	{
		throw std::runtime_error("add batches after populating");
	}

	deltas.template batch_merge_in<DeltaBatchValueMergeFn>(serial_cache);


	//deltas.

	/*	auto& m = b->get_delta_map();

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
			v.vectors.clear();
		}
	} */
}

void 
DeltaBatch::filter_invalid_deltas(TxBlock& txs)
{
	if (!populated)
	{
		throw std::runtime_error("cannot filter before populating with db values");
	}
	/*for (auto& [_, v] : deltas)
	{
		for (auto& dvs : v.vectors)
		{
			v.context->dv_all.add(std::move(*dvs));
		}
		// TODO switch away from singlethreaded mutator
		v.context -> mutator.filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(v.context -> dv_all, txs);
	} */

	throw std::runtime_error("umimpl");
	filtered = true;
}

void 
DeltaBatch::apply_valid_deltas(TxBlock const& txs)
{
	if (!filtered)
	{
		throw std::runtime_error("cannot apply before filtering");
	}
	throw std::runtime_error("unimpl");
	/*for (auto& [_, v] : deltas)
	{
		v.context -> mutator.apply_valid_deltas(v.context -> dv_all, txs);
	} */
	applied = true;
}

} /* scs */
