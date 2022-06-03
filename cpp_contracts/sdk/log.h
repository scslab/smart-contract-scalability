#pragma once

#include "sdk/macros.h"
#include "sdk/alloc.h"
#include "sdk/concepts.h"

namespace sdk
{

namespace detail
{

BUILTIN("log")
void log(uint32_t offset, uint32_t len);

} /* detail */

template<TriviallyCopyable T>
void log(T const& val)
{
	detail::log(sdk::to_offset(&val), sizeof(T));
}

} /* sdk */
