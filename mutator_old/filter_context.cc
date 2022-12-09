#include "mutator/filter_context.h"

namespace scs
{

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
				total_subtracted = INT64_MAX;
				return false;
			}

			total_subtracted += -delta;

			return (total_subtracted <= d.set_add_nonnegative_int64().set_value);
		}
		return true;
	}
};

class DeltaTypeClassApplierNNInt64SetAdd : public DeltaTypeClassApplier
{
	std::atomic<int64_t> total_subtracted;
	std::atomic<uint64_t> total_added;

public:

	DeltaTypeClassApplierNNInt64SetAdd(const DeltaTypeClass& tc)
		: DeltaTypeClassApplier(tc)
		, total_subtracted(0)
		, total_added(0)
		{}

	void
	add(StorageDelta const& d) override final
	{
		if (d.type() == DeltaType::DELETE_LAST)
		{
			is_deleted.test_and_set();
			return;
		}

		// by typeclass matching, d.type() == DeltaType::NONNEGATIVE_INT64_SET_ADD
		// and set_value matches

		int64_t delta = d.set_add_nonnegative_int64().delta;

		if (delta < 0)
		{
			total_subtracted.fetch_add(delta, std::memory_order_relaxed);
			return;
		} else
		{
			while(true)
			{
				uint64_t prev_value = total_added.load(std::memory_order_relaxed);

				if (__builtin_add_overflow_p(static_cast<uint64_t>(delta), prev_value, static_cast<uint64_t>(0)))
				{
					total_added.store(UINT64_MAX, std::memory_order_relaxed);
					return;
				} 
				else
				{
					if (total_added.compare_exchange_strong(prev_value, prev_value + static_cast<uint64_t>(delta), std::memory_order_relaxed))
					{
						return;
					}
				}
			}
		}
	}

	std::pair<bool, std::optional<StorageObject>>
	get_result() const override final
	{
		if (is_deleted.test())
		{
			return {true, std::nullopt};
		}

		auto const& base = tc.get_base_delta();

		StorageObject out;
		out.type(ObjectType::NONNEGATIVE_INT64);

		out.nonnegative_int64() = base.set_add_nonnegative_int64().set_value;
		out.nonnegative_int64() -= total_subtracted;

		uint64_t total_added_ = total_added.load(std::memory_order_relaxed);

		if (__builtin_add_overflow_p(total_added_, out.nonnegative_int64(), static_cast<int64_t>(0)))
		{
			out.nonnegative_int64() = INT64_MAX;
		} 
		else
		{
			out.nonnegative_int64() += total_added_;
		}

		return {false, out};
	}

};

class DeltaTypeClassFilterDelete : public DeltaTypeClassFilter
{
public:

	DeltaTypeClassFilterDelete(const DeltaTypeClass& tc)
		: DeltaTypeClassFilter(tc)
		{}

	bool
	add(StorageDelta const& d) override final
	{
		if (d.type() == DeltaType::DELETE_LAST)
		{
			return true;
		}
		if (d.type() == DeltaType::DELETE_FIRST)
		{
			return true;
		}

		throw std::runtime_error("storage delta type mismatch");
	}
};

class DeltaTypeClassApplierDelete : public DeltaTypeClassApplier
{
public:

	DeltaTypeClassApplierDelete(const DeltaTypeClass& tc)
		: DeltaTypeClassApplier(tc)
		{}

	void
	add(StorageDelta const& d) override final
	{
		if (d.type() == DeltaType::DELETE_LAST)
		{
			is_deleted.test_and_set();
			return;
		}
		if (d.type() == DeltaType::DELETE_FIRST)
		{
			is_deleted.test_and_set();
			return;
		}

		throw std::runtime_error("storage delta type mismatch");
	}

	std::pair<bool, std::optional<StorageObject>>
	get_result() const override final
	{
		if (is_deleted.test())
		{
			return {true, std::nullopt};
		}

		return {false, std::nullopt};
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
		case DeltaType::DELETE_FIRST:
		case DeltaType::DELETE_LAST:
			return std::unique_ptr<DeltaTypeClassFilter>{new DeltaTypeClassFilterDelete(base)};
	}

	throw std::runtime_error("TCFilter::make unimplemented type");
}

std::unique_ptr<DeltaTypeClassApplier> 
DeltaTypeClassApplier::make(DeltaTypeClass const& base)
{
	auto const& d = base.get_base_delta();

	switch(d.type())
	{
		case DeltaType::NONNEGATIVE_INT64_SET_ADD:
		{
			return std::unique_ptr<DeltaTypeClassApplier>{new DeltaTypeClassApplierNNInt64SetAdd(base)};
		}
		case DeltaType::DELETE_FIRST:
		case DeltaType::DELETE_LAST:
			return std::unique_ptr<DeltaTypeClassApplier>{new DeltaTypeClassApplierDelete(base)};
	}

	throw std::runtime_error("TCFilter::make unimplemented type");
}



} /* scs */
