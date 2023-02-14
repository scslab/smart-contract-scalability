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

#include "sdk/shared.h"

#include "sdk/types.h"

#include <array>
#include <cstdint>

namespace sdk
{

constexpr static 
StorageKey
make_static_key(uint64_t a, uint64_t b = 0, uint64_t c = 0, uint64_t d = 0)
{
	return scs::make_static_32bytes<StorageKey>(a, b, c, d);
}

/**
 * sdk key registry:
 *  0: replay cache id
 *  1: singlekey auth pk
 * 
 * contract-specific:
 * 	anything with b > 0 or c > 0 or d > 0
 */

constexpr static 
Address
make_static_address(uint64_t a, uint64_t b = 0, uint64_t c = 0, uint64_t d = 0)
{
	return scs::make_static_32bytes<Address>(a, b, c, d);
}


}