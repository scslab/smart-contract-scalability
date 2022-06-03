#pragma once

#include "sdk/macros.h"
#include "sdk/alloc.h"
#include "sdk/concepts.h"

namespace sdk
{

namespace detail
{

BUILTIN("get_calldata")
void get_calldata(uint32_t offset, uint32_t len);

} /* detail */

template<TriviallyCopyable T>
T get_calldata()
{
	T out;
	detail::get_calldata(to_offset(&out), sizeof(T));
	return out;
}

} /* sdk */
