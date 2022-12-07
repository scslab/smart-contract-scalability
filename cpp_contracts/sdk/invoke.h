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

BUILTIN("get_msg_sender")
void
get_msg_sender(
	uint32_t offset);

BUILTIN("get_self")
void
get_self(
	uint32_t offset);

BUILTIN("get_tx_hash")
void
get_tx_hash(
	uint32_t offset);

BUILTIN("get_invoked_tx_hash")
void
get_invoked_hash(
	uint32_t offset);

BUILTIN("return")
void
return_value(uint32_t offset, uint32_t len);

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

template<TriviallyCopyable ret = EmptyStruct>
ret invoke(Address const& addr, uint32_t methodname, uint8_t const* raw_calldata, uint32_t raw_calldata_len)
{
	ret out;
	uint32_t ret_len = 
		detail::invoke(
			to_offset(&addr), 
			methodname, 
			to_offset(raw_calldata),
			raw_calldata_len,
			to_offset(&out),
			sizeof(ret));
	if (ret_len != sizeof(ret))
	{
		abort();
	}
	return out;
}

Address
get_msg_sender()
{
	Address out;
	detail::get_msg_sender(to_offset(&out));
	return out;
}

Address
get_self()
{
	Address out;
	detail::get_self(to_offset(&out));
	return out;
}

Hash
get_tx_hash()
{
	Hash out;
	detail::get_tx_hash(to_offset(&out));
	return out;
}

Hash
get_invoked_hash()
{
	Hash out;
	detail::get_invoked_hash(to_offset(&out));
	return out;
}

template<TriviallyCopyable T>
void return_value(T const& r)
{
	detail::return_value(to_offset(&r), sizeof(r));
}

BUILTIN("get_block_number")
uint64_t
get_block_number();

} /* sdk */
