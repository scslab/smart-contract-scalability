#pragma once

#include <vector>
#include <memory>
#include <optional>

#include "object/object_mutator.h"

#include "object/delta_type_class.h"
#include "state_db/delta_vec.h"

namespace scs
{

struct DeltaBatchValueContext
{
	DeltaTypeClass typeclass;
	//ObjectMutator mutator;

	DeltaVector dv_all;

	DeltaBatchValueContext(DeltaTypeClass const& tc)
		: typeclass(tc)
	//	, mutator()
		, dv_all()
		{}
};

struct DeltaBatchValue
{
	std::vector<std::unique_ptr<DeltaVector>> vectors;
	
	// null in serial instances, nonnull otherwise
	std::unique_ptr<DeltaBatchValueContext> context;

	DeltaBatchValue()
		: vectors()
		, context()
		{}

	DeltaBatchValue(DeltaTypeClass const& tc)
		: vectors()
		, context(std::make_unique<DeltaBatchValueContext>(tc))
		{}
};

struct DeltaBatchValueMetadata
{
	//TODO compress into one 4 bit value?
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
		DeltaBatchValueMetadata meta;
		for (auto const& v : val.vectors)
		{
			meta.num_vectors++;
			meta.num_deltas += v->size();
		}
		return meta;
	}
};


} /* scs */
