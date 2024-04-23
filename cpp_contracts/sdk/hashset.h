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
#include <optional>

namespace sdk
{

void hashset_insert(
	StorageKey const& key,
	Hash const& hash,
	uint64_t threshold)
{
	detail::builtin_syscall(SYSCALLS::HS_INSERT,
		to_offset(&key), 
		to_offset(&hash),
		threshold, 0, 0, 0);
}

void
hashset_increase_limit(
	StorageKey const& key,
	uint16_t limit_increase)
{
	detail::builtin_syscall(SYSCALLS::HS_INC_LIMIT,
		to_offset(&key),
		limit_increase,
		0, 0, 0, 0);
}

void
hashset_clear(
	StorageKey const& key,
	uint64_t threshold)
{
	detail::builtin_syscall(SYSCALLS::HS_CLEAR,
		to_offset(&key),
		threshold, 0, 0, 0, 0);
}

uint32_t
hashset_get_size(
	StorageKey const& key)
{
	return detail::builtin_syscall(SYSCALLS::HS_GET_SIZE,
		to_offset(&key),
		0, 0, 0, 0, 0);
}

uint32_t
hashset_get_max_size(
	StorageKey const& key)
{
	return detail::builtin_syscall(SYSCALLS::HS_GET_MAX_SIZE,
		to_offset(&key),
		0, 0, 0, 0, 0);
}

/**
 *  if two things with same threshold, 
 * returns lowest index.
 * If none, returns UINT32_MAX
 **/
uint32_t
hashset_get_index_of(
	StorageKey const& key,
	uint64_t threshold)
{
	return detail::builtin_syscall(SYSCALLS::HS_GET_INDEX_OF,
		to_offset(&key),
		threshold,
		0, 0, 0, 0);
}

std::pair<uint64_t, Hash>
hashset_get_index(
	StorageKey const& key,
	uint32_t index)
{
	Hash out;
	uint64_t threshold = detail::builtin_syscall(SYSCALLS::HS_GET_INDEX,
		to_offset(&key),
		index,
		to_offset(&out),
		0, 0, 0);
	return {threshold, out};
}

} /* sdk */
