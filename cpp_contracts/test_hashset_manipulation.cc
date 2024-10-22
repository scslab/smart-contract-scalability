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

#include "sdk/alloc.h"
#include "sdk/replay_cache.h"
#include "sdk/semaphore.h"
#include "sdk/constexpr.h"
#include "sdk/raw_memory.h"
#include "sdk/invoke.h"
#include "sdk/calldata.h"
#include "sdk/auth_singlekey.h"
#include "sdk/log.h"
#include "sdk/flag.h"
#include "sdk/witness.h"
#include "sdk/crypto.h"
#include "sdk/general_storage.h"

#include <cstdint>

constexpr static sdk::StorageKey addr = sdk::make_static_key(0, 2);

sdk::Hash 
get_hash(uint64_t val)
{
	return sdk::hash(val);
}

EXPORT("pub00000000")
test_see_hs_writes_from_empty()
{
	sdk::print("start hashset manipulation tests phase 0");
	if (sdk::has_key(addr))
	{
		abort();
	}

	sdk::hashset_insert(addr, get_hash(0), 0);

	sdk::print("check size");
	assert(sdk::hashset_get_size(addr) == 1);
	sdk::print("check max size");
	assert(sdk::hashset_get_max_size(addr) == 64);

	sdk::hashset_insert(addr, get_hash(1), 1);

	assert(sdk::hashset_get_size(addr) == 2);

	sdk::hashset_insert(addr, get_hash(100), 0);
	sdk::hashset_insert(addr, get_hash(2), 2);
	sdk::hashset_insert(addr, get_hash(21), 2);

	assert(sdk::hashset_get_index_of(addr, 1) == 2);
	assert(sdk::hashset_get_index_of(addr, 0) == 3);

	assert(sdk::hashset_get_index(addr, 2).second == get_hash(1));

	sdk::hashset_clear(addr, 1);

	assert(sdk::hashset_get_size(addr) == 2);

	sdk::hashset_insert(addr, get_hash(3), 3);
	return 0;
}

EXPORT("pub01000000")
test_insert_after_clear_bad()
{
	sdk::hashset_clear(addr, 1);

	sdk::hashset_insert(addr, get_hash(0), 0);
	return 0;
}

// should run after method 0
EXPORT("pub02000000")
test_see_hs_writes_from_nonempty()
{
	if (!sdk::has_key(addr))
	{
		abort();
	}

	assert(sdk::hashset_get_size(addr) == 3);

	sdk::hashset_clear(addr, 1);

	assert(sdk::hashset_get_size(addr) == 3);

	assert(sdk::hashset_get_index_of(addr, 2) == 1);
	assert(sdk::hashset_get_index_of(addr, 3) == 0);

	assert(sdk::hashset_get_index(addr, 0).second == get_hash(3));

	sdk::hashset_insert(addr, get_hash(200), 2);

	sdk::hashset_clear(addr, 2);
	
	assert(sdk::hashset_get_size(addr) == 1);

	// can insert higher after a clear
	sdk::hashset_insert(addr, get_hash(4), 4);
	return 0;
}

