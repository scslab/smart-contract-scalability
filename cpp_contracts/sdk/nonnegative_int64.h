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


// creates if doesn't already exist
void
int64_add(StorageKey const& key, int64_t delta)
{
	detail::builtin_syscall(SYSCALLS::NNINT_ADD,
		to_offset(&key),
		delta,
		0, 0, 0, 0);
}

void 
int64_set_add(StorageKey const& key, int64_t set_value, int64_t delta)
{
	detail::builtin_syscall(SYSCALLS::NNINT_SET_ADD,
		to_offset(&key),
		set_value,
		delta,
		0, 0, 0);
}

void 
int64_set(StorageKey const& key, int64_t set_value)
{
	detail::builtin_syscall(SYSCALLS::NNINT_SET_ADD,
		to_offset(&key),
		set_value,
		0 /* delta */,
		0, 0, 0);
}

// returns 0 in default case where it does not exist
int64_t
int64_get(StorageKey const& key)
{
	return detail::builtin_syscall(SYSCALLS::NNINT_GET,
		to_offset(&key),
		0, 0, 0, 0, 0);
}

class NNInt64
{
	StorageKey key;

public:

	const int64_t set_value;
	int64_t delta;

	NNInt64(StorageKey&& key)
		: key(key)
		, set_value(int64_get(key))
		, delta(0)
		{}

	NNInt64(StorageKey&& key, int64_t set)
		: key(key)
		, set_value(set)
		, delta(0)
	{}

	int64_t safe_get_value() const
	{
		int64_t out;
		if (__builtin_add_overflow(set_value, delta, &out))
		{
			abort();
		}
		return out;
	}

	~NNInt64()
	{
		int64_set_add(key, set_value, delta);
	}

};

} /* sdk */
