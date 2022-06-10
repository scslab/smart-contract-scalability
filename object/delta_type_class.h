#pragma once

#include <optional>
#include "xdr/storage_delta.h"

namespace scs
{

/**
 * Each thread maintains per each locally modified key
 * a "vote" on the type of modifications that will
 * be acceptable during the batch filtering.
 * (this'll just be the highest-rank priority
 * that they see modifying a key, modulo the delete_first/last stuff).
 */
class DeltaTypeClass
{
	std::optional<DeltaPriority> priority;

	StorageDelta base_delta;

	bool is_lower_rank_than(StorageDelta const& new_delta, DeltaPriority const& new_priority) const;

public:

	DeltaTypeClass(StorageDelta const& origin_delta, DeltaPriority const& priority);

	// use when ordering doesn't matter, e.g. inside DeltaApplicator
	// when doing tx-local modifications
	DeltaTypeClass(StorageDelta const& origin_delta);

	// basically just comparing priorities,
	// taking deletion ordering into account.
	// if returns true, then the delta batch should swap in 
	// the new delta as the base typeclass
	bool is_lower_rank_than(DeltaTypeClass const& other) const;

	// reminder -- cannot be used during serial compute phase,
	// since we don't know what the final typeclass will be 
	// until we batch merge all threads' results
	bool accepts(StorageDelta const& delta) const;

	const StorageDelta&
	get_base_delta() const
	{
		return base_delta;
	}
};

} /* scs */
