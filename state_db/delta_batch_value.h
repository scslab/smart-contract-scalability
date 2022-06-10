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
	ObjectMutator mutator;

	DeltaVector dv_all;

	DeltaBatchValueContext(DeltaTypeClass const& tc)
		: typeclass(tc)
		, mutator()
		, dv_all()
		{}
};

struct DeltaBatchValue
{
	std::vector<std::unique_ptr<DeltaVector>> vectors;
	
	// null in serial instances, nonnull otherwise
	std::optional<DeltaBatchValueContext> context;

	DeltaBatchValue()
		: vectors()
		, context(std::nullopt)
		{}

	DeltaBatchValue(DeltaTypeClass const& tc)
		: vectors()
		, context(std::make_optional<DeltaBatchValueContext>(tc))
		{}
};


} /* scs */
