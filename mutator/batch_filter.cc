#include "mutator/batch_filter.h"

#include "state_db/delta_batch_value.h"

#include <cstdint>
#include "xdr/storage_delta.h"

#include "tx_block/tx_block.h"

namespace scs
{

class DeltaTypeClassFilter
{
	const DeltaTypeClass tc;

public:

	DeltaTypeClassFilter(const DeltaTypeClass& tc)
		: tc(tc)
		{}

	bool 
	matches_typeclass(const StorageDelta& d) const
	{
		return tc.accepts(d);
	}

	/**
	 * Important invariant:
	 * Once this returns false,
	 * no subsequent call depends
	 * on any previous call.
	 * That is to say,
	 * the results would not change
	 * if we had inserted some additional
	 * call to add() previously.
	 * 
	 * E.g. NNINT64: set_add stops allowing
	 * subtractions once the total subtracted
	 * exceeds the set amount.
	 * Adding more subtractions earlier
	 * does not allow a subsequent subtraction to proceed.
	 * 
	 * This is why, when a (e.g. large) sub call would
	 * overflow the set bound, we still update total_subtracted.
	 * (This helps enable a parallelizable implementation).
	 */
	virtual bool 
	add(StorageDelta const& d) = 0;

	static std::unique_ptr<DeltaTypeClassFilter> 
	make(DeltaTypeClass const& base);
};

class DeltaTypeClassFilterNNInt64SetAdd : public DeltaTypeClassFilter
{
	int64_t total_subtracted;

public:

	DeltaTypeClassFilterNNInt64SetAdd(const DeltaTypeClass& tc)
		: DeltaTypeClassFilter(tc)
		, total_subtracted(0)
		{}

	bool
	add(StorageDelta const& d) override final
	{
		if (d.type() == DeltaType::DELETE_LAST)
		{
			return true;
		}

		// by typeclass matching, d.type() == DeltaType::NONNEGATIVE_INT64_SET_ADD
		// and set_value matches
		/* 
		if (d.type() != DeltaType::NONNEGATIVE_INT64_SET_ADD)
		{
			throw std::runtime_error("should have been handled by typeclass matching");
		} */

		int64_t delta = d.set_add_nonnegative_int64().delta;

		if (delta < 0)
		{
			if (delta == INT64_MIN)
			{
				throw std::runtime_error("INT64_MIN shouldn't have gotten through to filtering step");
			}

			if (-delta > d.set_add_nonnegative_int64().set_value)
			{
				throw std::runtime_error("invalid set_add shoudln't have gotten past computation step");
			}

			if (__builtin_add_overflow_p(-delta, total_subtracted, static_cast<int64_t>(0)))
			{
				return false;
			}

			total_subtracted += -delta;

			return (total_subtracted <= d.set_add_nonnegative_int64().set_value);
		}

		return true;
	}
};

std::unique_ptr<DeltaTypeClassFilter> 
DeltaTypeClassFilter::make(DeltaTypeClass const& base)
{
	auto const& d = base.get_base_delta();

	switch(d.type())
	{
		case DeltaType::NONNEGATIVE_INT64_SET_ADD:
		{
			return std::unique_ptr<DeltaTypeClassFilter>{new DeltaTypeClassFilterNNInt64SetAdd(base)};
		}
	}

	throw std::runtime_error("TCFilter::make unimplemented type");
}

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
		//	std::printf("fail from compute check\n");
			continue;
		}

		if (!filter -> matches_typeclass(d))
		{
	//		std::printf("mismatch typeclass \n");
			txs.template invalidate<TransactionFailurePoint::CONFLICT_PHASE_1>(p.tx_hash);
			continue;
		}

		if (!filter -> add(d))
		{
	//		std::printf("filter add fail\n");
			txs.template invalidate<TransactionFailurePoint::CONFLICT_PHASE_1>(p.tx_hash);
			continue;
		}
	//	std::printf("success\n");
	}
}




} /* scs */
