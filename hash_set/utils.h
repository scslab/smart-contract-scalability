#pragma once

#include <algorithm>
#include "xdr/storage.h"
#include "object/comparators.h"

namespace scs
{

[[maybe_unused]]
static void normalize_hashset(HashSet& hs)
{
	auto& h_list = hs.hashes;
	std::sort(h_list.begin(), h_list.end(), std::greater<HashSetEntry>());
}

}