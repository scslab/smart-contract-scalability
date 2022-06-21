#include "filter/filter_context.h"

#include "state_db/delta_vec.h"

#include "object/delta_type_class.h"
#include "tx_block/tx_block.h"


namespace scs
{

FilterContext::FilterContext(DeltaTypeClass const& dtc)
	: dtc(dtc)
	{}


bool 
FilterContext::check_valid_dtc()
{
	return dtc.get_valence().tv.type() != TypeclassValence::TV_ERROR;
}


void 
FilterContext::add_vec(DeltaVector const& v)
{
	if (!check_valid_dtc())
	{
		return;
	}

	add_vec_when_tc_valid(v);
}

void 
FilterContext::invalidate_entire_vector(DeltaVector const& v, TxBlock& txs)
{
	auto const& vs = v.get();
	for (auto const& [d, h] : vs)
	{
		txs.template invalidate<TransactionFailurePoint::CONFLICT_PHASE_1>(h);
	}
}

void 
FilterContext::prune_invalid_txs(DeltaVector const& v, TxBlock& txs)
{
	if (!check_valid_dtc())
	{
		invalidate_entire_vector(v, txs);
		return;
	}

	prune_invalid_txs_when_tc_valid(v, txs);
}


class NNInt64FilterContext : public FilterContext
{

	std::atomic<int64_t> subtracted_amount;

	std::atomic_flag local_failure;

public:

	NNInt64FilterContext(DeltaTypeClass const& dtc)
		: FilterContext(dtc)
		, subtracted_amount(0)
		, local_failure()
		{}

	void 
	add_vec_when_tc_valid(DeltaVector const& v) override final
	{
		int64_t local_subtracted_amount = 0;

		auto const& vs = v.get();
		for (auto const& [d,_] : vs)
		{
			// by typeclass, all d have same set_value and are of type set_add_nn_int64

			int64_t delta = d.set_add_nonnegative_int64().delta;

			if (delta < 0)
			{
				if (__builtin_add_overflow_p(delta, local_subtracted_amount, static_cast<int64_t>(0)))
				{
					local_failure.test_and_set();
					break;
				}
				local_subtracted_amount += delta;
			}
		}

		while(true)
		{
			int64_t cur_value = subtracted_amount.load(std::memory_order_relaxed);

			if (__builtin_add_overflow_p(cur_value, local_subtracted_amount, static_cast<int64_t>(0)))
			{
				local_failure.test_and_set();
				return;
			}
			int64_t res = cur_value + local_subtracted_amount;
			if (subtracted_amount.compare_exchange_strong(cur_value, res, std::memory_order_relaxed))
			{
				return;
			}
		}
	}

	void
	prune_invalid_txs_when_tc_valid(DeltaVector const& v, TxBlock& txs) override final
	{
		bool failed = false;

		if (local_failure.test())
		{
			failed = true;
		}

		int64_t base_val = dtc.get_valence().tv.set_value();

		if (base_val < subtracted_amount)
		{
			failed = true;
		}

		if (failed)
		{
			auto const& vs = v.get();
			for (auto const& [d, h] : vs)
			{
				int64_t delta = d.set_add_nonnegative_int64().delta;

				if (delta < 0)
				{
					txs.template invalidate<TransactionFailurePoint::CONFLICT_PHASE_1>(h);
				}
			}
		}
	}
};

std::unique_ptr<FilterContext> 
make_filter_context(
	DeltaTypeClass const& dtc)
{
	switch(dtc.get_valence().tv.type())
	{
		case TypeclassValence::TV_FREE:
			// nothing to filter here
			throw std::runtime_error("free unimpl");
		case TypeclassValence::TV_NONNEGATIVE_INT64_SET:
			return std::make_unique<NNInt64FilterContext>(dtc);
		default:
			throw std::runtime_error("unimpl filter");
	}
}

} /* scs */
