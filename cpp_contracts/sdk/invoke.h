#pragma once

#include "sdk/macros.h"
#include "sdk/concepts.h"
#include "sdk/alloc.h"
#include "sdk/types.h"
#include "sdk/concepts.h"

#include <cstdint>

namespace sdk
{

namespace detail
{

BUILTIN("invoke")
uint32_t
invoke(
	uint32_t addr_offset, 
	uint32_t methodname, 
	uint32_t calldata_offset, 
	uint32_t calldata_len,
	uint32_t return_offset,
	uint32_t return_len);

} /* detail */

// only works for fixed-size types here
// vector types or other inputs need a different function
template<TriviallyCopyable calldata, TriviallyCopyable ret = EmptyStruct>
ret invoke(Address const& addr, uint32_t methodname, calldata const& data)
{
	ret out;
	uint32_t ret_len = 
		detail::invoke(
			to_offset(&addr), 
			methodname, 
			to_offset(&data),
			sizeof(calldata),
			to_offset(&out),
			sizeof(ret));
	if (ret_len != sizeof(ret))
	{
		abort();
	}
	return out;
}

} /* sdk */
