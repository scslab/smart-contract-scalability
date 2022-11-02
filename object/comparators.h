#pragma once

#include <compare>

namespace scs
{

struct HashSetEntry;

std::strong_ordering 
operator<=>(const HashSetEntry& d1, const HashSetEntry& d2);

} // scs
