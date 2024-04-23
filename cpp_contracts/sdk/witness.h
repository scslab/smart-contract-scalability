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

#include "sdk/concepts.h"
#include "sdk/alloc.h"
#include "sdk/types.h"
#include "sdk/concepts.h"
#include "sdk/syscall.h"

#include <cstdint>
#include <optional>

namespace sdk
{

template<TriviallyCopyable T>
T get_witness(uint64_t witness)
{
	T out;
	detail::builtin_syscall(SYSCALLS::WITNESS_GET,
		witness, to_offset(&out), sizeof(T),
		0, 0, 0);
	return out;
}

uint32_t get_witness_len(uint64_t wit_idx)
{
	return detail::builtin_syscall(SYSCALLS::WITNESS_GET_LEN,
		wit_idx, 0, 0, 0, 0, 0);
}

} // namespace sdk


