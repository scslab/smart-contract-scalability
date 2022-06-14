#pragma once

#include "xdr/storage.h"
#include "xdr/storage_delta.h"

#include "object/comparators.h"

#include "object/delta_type_class.h"

#include <set>

namespace scs
{

class DeltaVector
{
	using set_t = std::set<std::pair<StorageDelta, DeltaPriority>, std::greater<>>;

	set_t deltas;
public:

	DeltaVector()
		: deltas()
		{}

	DeltaVector(const DeltaVector& other) = delete;

	void add_delta(StorageDelta&& d, DeltaPriority&& p);

	void add(DeltaVector&& other);

	const set_t& get_sorted_deltas() const
	{
		return deltas;
	}

	DeltaTypeClass get_typeclass_vote() const;

	size_t size() const
	{
		return deltas.size();
	}
};

} /* scs */
