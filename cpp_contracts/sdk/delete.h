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

//BUILTIN("delete_key_first")
//void 
//delete_first(
//	uint32_t key_offset);

BUILTIN("delete_key_last")
void
delete_last(
	uint32_t key_offset);

} /* detail */

//void delete_first(StorageKey const& key)
//{
//	detail::delete_first(to_offset(&key));
//}

void delete_last(StorageKey const& key)
{
	detail::delete_last(to_offset(&key));
}

} /* sdk */
