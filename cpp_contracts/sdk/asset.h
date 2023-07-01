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

BUILTIN("asset_add")
void
asset_add(
	uint32_t key_offset,
	/* key_len = 32 */
	int64_t delta);

BUILTIN("asset_get")
uint64_t
asset_get(
	uint32_t key_offset
	/* key_len = 32 */);

} /* detail */

// creates if doesn't already exist
void
asset_add(StorageKey const& key, int64_t delta)
{
	detail::asset_add(
		to_offset(&key),
		delta);
}

uint64_t
asset_get(StorageKey const& key)
{
	return detail::asset_get(
		to_offset(&key));
}

} /* sdk */
