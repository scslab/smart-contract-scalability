#pragma once

#include "sdk/macros.h"
#include "sdk/alloc.h"
#include "sdk/concepts.h"

namespace sdk
{

namespace detail
{

BUILTIN("get_calldata")
void get_calldata(uint32_t offset, uint32_t start_offset, uint32_t end_offset);

} /* detail */

template<TriviallyCopyable T>
T get_calldata()
{
	T out;
	detail::get_calldata(to_offset(&out), 0, sizeof(T));
	return out;
}

BUILTIN("get_calldata_len")
uint32_t get_calldata_len();

} /* sdk */
