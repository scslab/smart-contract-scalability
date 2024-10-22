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

#include "sdk/hashset.h"
#include "sdk/calldata.h"
#include "sdk/invoke.h"
#include "sdk/crypto.h"

struct calldata_0
{
	sdk::StorageKey key;
	uint64_t value;
};

EXPORT("pub00000000")
hashset_insert()
{
	auto calldata = sdk::get_calldata<calldata_0>();
	auto hash = sdk::hash(reinterpret_cast<uint8_t*>(&(calldata.value)), sizeof(uint64_t));

	sdk::hashset_insert(calldata.key, hash, 0);
	return 0;
}

