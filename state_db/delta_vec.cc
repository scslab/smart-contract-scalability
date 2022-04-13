#include "state_db/delta_vec.h"

using xdr::operator==;

namespace scs
{

std::strong_ordering operator<=>(const DeltaPriority& d1, const DeltaPriority& d2)
{
	auto priority = d1.custom_priority <=> d2.custom_priority;
	if (priority != std::strong_ordering::equal)
	{
		return priority;
	}

	auto gas = d1.gas_bid <=> d2.gas_bid;
	if (gas != std::strong_ordering::equal)
	{
		return gas;
	}

	auto hash = d1.tx_hash <=> d2.tx_hash;
	if (hash != std::strong_ordering::equal)
	{
		return hash;
	}

	return d1.delta_id_number <=> d2.delta_id_number;
}

std::weak_ordering operator<=>(const std::pair<StorageDelta, DeltaPriority>& d1, const std::pair<StorageDelta, DeltaPriority>& d2)
{
	auto res = d1.second <=> d2.second;
	if (res != std::strong_ordering::equal)
	{
		return res;
	}
	return std::weak_ordering::equivalent;
}


void
DeltaVector::add_delta(StorageDelta&& d, DeltaPriority&& p)
{
	deltas.insert(std::make_pair(std::move(d), std::move(p)));
}


} /* scs */
