#pragma once

#include "sdk/types.h"

#include <array>
#include <cstdint>

namespace sdk
{

namespace detail
{

constexpr static 
void write_uint64_t(uint8_t* out, uint64_t value)
{
	for (size_t i = 0; i < 8; i++)
	{
		out[i] = (value >> (8*i)) & 0xFF;
	}
}

}

constexpr static 
StorageKey
make_static_key(uint64_t a, uint64_t b = 0, uint64_t c = 0, uint64_t d = 0)
{
	StorageKey out = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t* data = out.data();
	detail::write_uint64_t(data, a);
	detail::write_uint64_t(data + 8, b);
	detail::write_uint64_t(data + 16, c);
	detail::write_uint64_t(data + 24, d);
	return out;
}

/**
 * sdk key registry:
 *  0: replay cache id
 * 
 * contract-specific:
 * 	anything with b > 0 or c > 0 or d > 0
 */

}