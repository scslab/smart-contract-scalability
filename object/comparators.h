#pragma once

#include <compare>
#include <utility>

namespace scs
{

class DeltaPriority;
class StorageDelta;

std::strong_ordering operator<=>(const DeltaPriority& d1, const DeltaPriority& d2);
std::weak_ordering operator<=>(const std::pair<StorageDelta, DeltaPriority>& d1, const std::pair<StorageDelta, DeltaPriority>& d2);

} /* scs */
