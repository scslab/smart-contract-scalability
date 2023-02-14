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

#include <cstdint>
#include <vector>

namespace sdk
{

namespace detail
{

BUILTIN("external_call")
void
external_call(
	uint32_t target_addr,
	/* addr_len = 32 */
	uint32_t call_data_offset,
	uint32_t call_data_len,
	uint32_t response_offset,
	uint32_t response_max_len);
}

template<TriviallyCopyable C, TriviallyCopyable R = EmptyStruct>
R 
external_call(const Address& addr, C const& calldata)
{
	R out;

	detail::external_call(
		to_offset(&addr),
		to_offset(&calldata),
		sizeof(C),
		to_offset(&out),
		sizeof(R));
	return out;
}

template<VectorLike C>
std::vector<uint8_t>
external_call(const Address& addr, C const& calldata, uint32_t output_max_len)
{
	std::vector<uint8_t> out;
	out.resize(output_max_len);

	detail::external_call(
		to_offset(&addr),
		to_offset(calldata.data()),
		calldata.size(),
		to_offset(out.data()),
		output_max_len);
	return out;
}

}