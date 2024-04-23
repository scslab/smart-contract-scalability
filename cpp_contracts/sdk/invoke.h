/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "sdk/macros.h"
#include "sdk/concepts.h"
#include "sdk/alloc.h"
#include "sdk/types.h"
#include "sdk/concepts.h"

#include "sdk/syscall.h"

#include <cstdint>
#include <type_traits>

namespace sdk
{

// only works for fixed-size types here
// vector types or other inputs need a different function
template<TriviallyCopyable calldata, TriviallyCopyable ret = EmptyStruct>
ret invoke(Address const& addr, uint32_t methodname, calldata const& data)
{
	ret out;
	uint32_t ret_len = 
		detail::builtin_syscall(SYSCALLS::INVOKE,
			to_offset(&addr),
			methodname, 
			to_offset(&data),
			sizeof(calldata),
			to_offset(&out),
			sizeof(ret));
	if constexpr (!std::is_same_v<ret, EmptyStruct>) {
		if (ret_len != sizeof(ret))
		{
			abort();
		}
	} else {
		if (ret_len != 0)
		{
			abort();
		}
	}
	return out;
}

template<TriviallyCopyable ret = EmptyStruct>
ret invoke(Address const& addr, uint32_t methodname, uint8_t const* raw_calldata, uint32_t raw_calldata_len)
{
	ret out;
	uint32_t ret_len = 
		detail::builtin_syscall(SYSCALLS::INVOKE,
			to_offset(&addr), 
			methodname, 
			to_offset(raw_calldata),
			raw_calldata_len,
			to_offset(&out),
			sizeof(ret));
	if constexpr (!std::is_same_v<ret, EmptyStruct>) {
		if (ret_len != sizeof(ret))
		{
			abort();
		}
	} else {
		if (ret_len != 0)
		{
			abort();
		}
	}
	return out;
}

Address
get_msg_sender()
{
	Address out;
	detail::builtin_syscall(SYSCALLS::GET_SENDER,
		to_offset(&out), 0, 0, 0, 0, 0);
	return out;
}

Address
get_self()
{
	Address out;
	detail::builtin_syscall(SYSCALLS::GET_SELF_ADDR,
		to_offset(&out), 0, 0, 0, 0, 0);
	return out;
}

Hash
get_tx_hash()
{
	Hash out;
	detail::builtin_syscall(SYSCALLS::GET_SRC_TX_HASH,
		to_offset(&out), 0, 0, 0, 0, 0);
	return out;
}

Hash
get_invoked_hash()
{
	Hash out;
	detail::builtin_syscall(SYSCALLS::GET_INVOKED_TX_HASH,
		to_offset(&out), 0, 0, 0, 0, 0);
	return out;
}

template<TriviallyCopyable T>
void return_value(T const& r)
{
	detail::builtin_syscall(SYSCALLS::RETURN,
		to_offset(&r),
		sizeof(T), 0, 0, 0, 0);
}

uint64_t
get_block_number()
{
	return detail::builtin_syscall(SYSCALLS::GET_BLOCK_NUMBER,
		0, 0, 0, 0, 0, 0);
}

} /* sdk */
