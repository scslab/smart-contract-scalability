#include "mutator/batch_filter.h"

#include "state_db/delta_batch_value.h"

#include <cstdint>
#include "xdr/storage_delta.h"

#include "tx_block/tx_block.h"

namespace scs
{

void 
filter_serial(DeltaBatchValue& v, TxBlock& txs)
{
	if (!v.context)
	{
		throw std::runtime_error("cannot filter without context");
	}

	for (auto& dvs : v.vectors)
	{
		v.context->dv_all.add(std::move(*dvs));
	}

	std::printf("n deltas: %lu\n", v.context -> dv_all.size());

	auto filter = DeltaTypeClassFilter::make(v.context -> typeclass);

	for (auto const& [d, p] : v.context -> dv_all.get_sorted_deltas())
	{
		if (!txs.is_valid(TransactionFailurePoint::COMPUTE, p.tx_hash))
		{
			continue;
		}

		if (!filter -> matches_typeclass(d))
		{
			txs.template invalidate<TransactionFailurePoint::CONFLICT_PHASE_1>(p.tx_hash);
			continue;
		}

		if (!filter -> add(d))
		{
			txs.template invalidate<TransactionFailurePoint::CONFLICT_PHASE_1>(p.tx_hash);
			continue;
		}
	}
}




} /* scs */
