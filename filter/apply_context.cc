#include "filter/apply_context.h"

#include <cstdint>
#include <atomic>

#include "object/delta_type_class.h"
#include "state_db/delta_vec.h"
#include "tx_block/tx_block.h"

namespace scs
{

ApplyContext::ApplyContext(DeltaTypeClass const& dtc)
	: dtc(dtc)
	{}

std::optional<StorageObject>
ApplyContext::get_object() const
{
	if (dtc.get_valence().deleted_last) {
		return std::nullopt;
	}
	return get_object_();
}

class NNInt64ApplyContext : public ApplyContext
{
	std::atomic<int64_t> subtracted_amount;
	std::atomic<uint64_t> added_amount;


public:

	NNInt64ApplyContext(DeltaTypeClass const& dtc)
		: ApplyContext(dtc)
		, subtracted_amount(0)
		, added_amount(0)
		{}

	void add_vec(DeltaVector const& v, const TxBlock& txs) override final
	{
		auto const& vs = v.get();
		int64_t local_subtracted_amount = 0;
		uint64_t local_added_amount = 0;

		for (auto const& [d, h] : vs)
		{
			if (txs.is_valid(TransactionFailurePoint::FINAL, h))
			{
				int64_t delta = d.set_add_nonnegative_int64().delta;

				if (delta < 0)
				{
					local_subtracted_amount -= delta;
				}
				else
				{
					if (__builtin_add_overflow_p(local_added_amount, delta, static_cast<uint64_t>(0)))
					{
						local_added_amount = UINT64_MAX;
					} else
					{
						local_added_amount += delta;
					}
				}
			}
		}

		subtracted_amount.fetch_add(local_subtracted_amount);

		while(true)
		{
			uint64_t add = added_amount.load(std::memory_order_relaxed);
			if (__builtin_add_overflow_p(add, local_added_amount, static_cast<uint64_t>(0)))
			{
				added_amount.store(UINT64_MAX, std::memory_order_relaxed);
				return;
			}
			uint64_t res = add + local_added_amount;
			if (added_amount.compare_exchange_strong(add, res, std::memory_order_relaxed))
			{
				return;
			}
		}
	}

	std::optional<StorageObject> 
	get_object_() const override final
	{
		StorageObject out;
		out.type(ObjectType::NONNEGATIVE_INT64);

		int64_t base = dtc.get_valence().tv.set_value();
		base -= subtracted_amount;
		uint64_t add = added_amount.load(std::memory_order_relaxed);

		if (__builtin_add_overflow_p(add, base, static_cast<int64_t>(0)))
		{
			base = INT64_MAX;
		} else
		{
			base = base + add;
		}
		out.nonnegative_int64() = base;
		return out;
	}
};

std::unique_ptr<ApplyContext> 
make_apply_context(
	DeltaTypeClass const& dtc)
{
	switch(dtc.get_valence().tv.type())
	{
		case TypeclassValence::TV_FREE:
			// nothing to filter here
			throw std::runtime_error("free unimpl");
		case TypeclassValence::TV_NONNEGATIVE_INT64_SET:
			return std::make_unique<NNInt64ApplyContext>(dtc);
		default:
			throw std::runtime_error("unimpl filter");
	}
}


} /* scs */
