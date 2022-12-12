#pragma once

#include <cstdint>

namespace scs
{

// must be greater than # of threads created in one run
constexpr static uint32_t TLCACHE_SIZE = 512;
constexpr static uint32_t TLCACHE_BITS_REQUIRED = 9; // 512 = 2^9

static_assert (static_cast<uint32_t>(1) << TLCACHE_BITS_REQUIRED == TLCACHE_SIZE, "mismatch");

}
