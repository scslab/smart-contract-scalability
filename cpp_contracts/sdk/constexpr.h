#pragma once

#include "sdk/shared.h"

#include "sdk/types.h"

#include <array>
#include <cstdint>

namespace sdk
{

constexpr static 
StorageKey
make_static_key(uint64_t a, uint64_t b = 0, uint64_t c = 0, uint64_t d = 0)
{
	return scs::make_static_32bytes<StorageKey>(a, b, c, d);
}

/**
 * sdk key registry:
 *  0: replay cache id
 * 
 * contract-specific:
 * 	anything with b > 0 or c > 0 or d > 0
 */

constexpr static 
Address
make_static_address(uint64_t a, uint64_t b = 0, uint64_t c = 0, uint64_t d = 0)
{
	return scs::make_static_32bytes<Address>(a, b, c, d);
}


}