#pragma once

#include "sdk/macros.h"
#include "sdk/concepts.h"
#include "sdk/alloc.h"
#include "sdk/types.h"
#include "sdk/concepts.h"

#include <cstdint>
#include <optional>

namespace sdk
{

namespace detail
{

BUILTIN("hash")
void
hash(
	uint32_t input_offset,
	uint32_t input_len,
	uint32_t output_offset);

} /* detail */

Hash hash(uint8_t* data, uint32_t size)
{
	Hash out;
	detail::hash(to_offset(data), size, to_offset(hash.data()));
	return out;
}

template <VectorLike T>
Hash hash(T const& object)
{
	return hash(object.data(), object.size());
}

} /* sdk */
