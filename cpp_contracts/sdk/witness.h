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

BUILTIN("get_witness")
void
get_witness(
	uint64_t wit_idx,
	uint32_t out_offset,
	uint32_t max_len);

} // namespace detail

template<TriviallyCopyable T>
T get_witness(uint64_t witness)
{
	T out;
	detail::get_witness(witness, to_offset(&out), sizeof(T));
	return out;
}

BUILTIN("get_witness_len")
uint32_t
get_witness_len(
	uint64_t wit_idx);

} // namespace sdk


