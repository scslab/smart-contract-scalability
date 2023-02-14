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
#include "sdk/alloc.h"
#include "sdk/concepts.h"

#include <type_traits>

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
	if constexpr (std::is_empty<T>::value)
	{
		return T{};
	}
	
	T out;
	detail::get_calldata(to_offset(&out), 0, sizeof(T));
	return out;
}

void
get_calldata_slice(uint8_t* out, uint32_t start_offset, uint32_t end_offset)
{
	detail::get_calldata(to_offset(out), start_offset, end_offset);
}

BUILTIN("get_calldata_len")
uint32_t get_calldata_len();

} /* sdk */
