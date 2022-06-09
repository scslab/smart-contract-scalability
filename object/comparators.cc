#include "object/comparators.h"

#include "xdr/storage.h"
#include "xdr/storage_delta.h"

namespace scs
{

std::strong_ordering operator<=>(const DeltaPriority& d1, const DeltaPriority& d2)
{
	auto priority = d1.custom_priority <=> d2.custom_priority;
	if (priority != std::strong_ordering::equal)
	{
		return priority;
	}

	auto gas = d1.gas_rate_bid <=> d2.gas_rate_bid;
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

std::weak_ordering type_ordering_compare(const DeltaType& dt1, const DeltaType& dt2)
{
	if (dt1 == dt2) {
		return std::weak_ordering::equivalent;
	}
	if (dt1 == DeltaType::DELETE_FIRST)
	{
		return std::weak_ordering::greater;
	}
	if (dt2 == DeltaType::DELETE_FIRST)
	{
		return std::weak_ordering::less;
	}
	if (dt1 == DeltaType::DELETE_LAST)
	{
		return std::weak_ordering::less;
	}
	if (dt2 == DeltaType::DELETE_LAST)
	{
		return std::weak_ordering::greater;
	}
	return std::weak_ordering::equivalent;
}

std::weak_ordering operator<=>(const std::pair<StorageDelta, DeltaPriority>& d1, const std::pair<StorageDelta, DeltaPriority>& d2)
{
	std::printf("invoking comparison %lu %lu\n", d1.first.type(), d2.first.type());
	auto t_res = type_ordering_compare(d1.first.type(), d2.first.type());
	if (t_res != std::weak_ordering::equivalent)
	{
		std::printf("nequal via type %ld %ld\n", d1.first.type(), d2.first.type());
		return t_res;
	}

	auto res = d1.second <=> d2.second;
	if (res != std::strong_ordering::equal)
	{
		std::printf("nequal via priority\n");
		return res;
	}
	std::printf("equal overall\n");
	return std::weak_ordering::equivalent;
}


} /* scs */
