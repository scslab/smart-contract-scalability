#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <optional>

#include "object/object_mutator.h"

#include "object/delta_type_class.h"
#include "state_db/delta_vec.h"

#include "filter/filter_context.h"
#include "filter/apply_context.h"

#include "utils/atomic_singleton.h"

namespace scs
{


struct DeltaBatchValueContext
{

	std::unique_ptr<FilterContext> filter;
	std::unique_ptr<ApplyContext> applier;
	//std::unique_ptr<DeltaTypeClassApplier> applier;

	DeltaBatchValueContext(DeltaTypeClass const& tc)
		: filter(make_filter_context(tc))
		, applier(make_apply_context(tc))
		{}
};

class DeltaBatchValue
{

	std::unique_ptr<AtomicSingleton<DeltaBatchValueContext>> context;
	// null in serial instances, nonnull otherwise

public:

	std::vector<DeltaVector> vectors;
	DeltaTypeClass tc;
	
	DeltaBatchValue()
		: context()
		, vectors()
		, tc()

		{
			vectors.emplace_back();
		}

	DeltaBatchValueContext& get_context()
	{
		return context -> get(tc);
	}
	
	const DeltaBatchValueContext& get_context() const
	{
		return context -> get(tc);
	}
};

struct DeltaBatchValueMetadata
{
	//TODO compress into one 4 byte value?
	// It should be _ok_ now, with only adding 8 bytes
	int32_t num_deltas = 0;
	int32_t num_vectors = 0;

	DeltaBatchValueMetadata& 
	operator+=(const DeltaBatchValueMetadata& other)
	{
		num_deltas += other.num_deltas;
		num_vectors += other.num_vectors;
		return *this;
	}

	friend DeltaBatchValueMetadata operator-(
		DeltaBatchValueMetadata lhs, 
		DeltaBatchValueMetadata const& rhs)
	{
		lhs.num_deltas -= rhs.num_deltas;
		lhs.num_vectors -= rhs.num_vectors;
		return lhs;
	}

	DeltaBatchValueMetadata operator-() const
	{
		return DeltaBatchValueMetadata
		{
			.num_deltas = -this->num_deltas,
			.num_vectors = -this->num_vectors
		};
	}

	constexpr static DeltaBatchValueMetadata zero()
	{
		return DeltaBatchValueMetadata{
			.num_deltas = 0,
			.num_vectors = 0
		};
	}

	static 
	DeltaBatchValueMetadata
	from_value(DeltaBatchValue const& val)
	{
		DeltaBatchValueMetadata meta = DeltaBatchValueMetadata::zero();
		for (auto const& v : val.vectors)
		{
			meta.num_vectors++;
			meta.num_deltas += v.size();
		}
		return meta;
	}
};


} /* scs */
