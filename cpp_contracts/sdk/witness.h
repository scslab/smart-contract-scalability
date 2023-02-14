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


