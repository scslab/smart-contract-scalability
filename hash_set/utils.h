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

[[maybe_unused]]
static void clear_hashset(HashSet& hs, uint64_t threshold)
{
	// TODO they're sorted, so we can just truncate the list and avoid the iterator overhead
	auto& h_list = hs.hashes;
    uint32_t min = 0, max = h_list.size();
    uint32_t med = 0;

    while(true)
    {
        med = (max + min) / 2;
        if (max == min)
        {
            break;
        }

        if (h_list[med].index > threshold)
        {
            min = med + 1;
        } 
        else 
        {
            max = med;
        }
    }
    h_list.resize(med);

/*
	for (auto it = h_list.begin(); it != h_list.end();)
    {
        if (it -> index <= threshold)
        {
            it = h_list.erase(it);
        } else
        {
            ++it;
        }
    } */
}

}