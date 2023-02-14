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

#include "sdk/types.h"
#include "sdk/nonnegative_int64.h"
#include "sdk/delete.h"

namespace sdk
{

template<uint32_t count = 1>
class Semaphore
{
	const sdk::StorageKey key;

public:

	constexpr Semaphore(sdk::StorageKey k)
		: key(std::move(k))
		{}

	void acquire()
	{
		int64_set_add(key, count, -1);
	}
};

template<uint32_t count = 1>
class TransientSemaphore
{
	const sdk::StorageKey key;

public:

	constexpr TransientSemaphore(sdk::StorageKey k)
		: key(std::move(k))
		{}

	void acquire()
	{
		int64_set_add(key, count, -1);
	}

	~TransientSemaphore()
	{
		delete_last(key);
	}
};

}