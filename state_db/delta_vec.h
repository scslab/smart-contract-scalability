#pragma once

#include "xdr/storage.h"
#include "xdr/storage_delta.h"

#include <compare>
#include <set>

namespace scs
{

std::strong_ordering operator<=>(const DeltaPriority& d1, const DeltaPriority& d2);
std::weak_ordering operator<=>(const std::pair<StorageDelta, DeltaPriority>& d1, const std::pair<StorageDelta, DeltaPriority>& d2);

class DeltaVector
{
	using set_t = std::set<std::pair<StorageDelta, DeltaPriority>, std::greater<>>;

	set_t deltas;
public:

	void add_delta(StorageDelta&& d, DeltaPriority&& p);

	void add(DeltaVector&& other)
	{
		deltas.merge(other.deltas);
		if (other.deltas.size() != 0)
		{
			throw std::runtime_error("collision within delta_vec");
		}
	}

	const set_t& get_sorted_deltas() const
	{
		return deltas;
	}
};

} /* scs */
