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


template<TriviallyCopyable T>
void set_raw_memory(const StorageKey& key, const T& value)
{
	detail::builtin_syscall(SYSCALLS::RAW_MEM_SET,
		to_offset(key.data()),
		to_offset(&value),
		sizeof(T),
		0, 0, 0);
}

template<TriviallyCopyable T>
std::optional<T>
get_raw_memory_opt(const StorageKey& key)
{
	std::optional<T> out = T{};
	int64_t res = detail::builtin_syscall(SYSCALLS::RAW_MEM_GET,
		to_offset(key.data()),
		to_offset(&out),
		sizeof(T), 0, 0, 0);

	if (res < 0)
	{
		return std::nullopt;
	}
	return out;
}

template<TriviallyCopyable T>
T
get_raw_memory(const StorageKey& key)
{
	T out;
	int64_t res = detail::builtin_syscall(SYSCALLS::RAW_MEM_GET,
		to_offset(key.data()),
		to_offset(&out),
		sizeof(T), 0, 0, 0);

	if (res < 0)
	{
		abort();
	}
	return out;
}

} /* sdk */
