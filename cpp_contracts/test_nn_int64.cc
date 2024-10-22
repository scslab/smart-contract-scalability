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

#include "sdk/calldata.h"
#include "sdk/log.h"
#include "sdk/raw_memory.h"
#include "sdk/invoke.h"
#include "sdk/nonnegative_int64.h"

struct calldata_0 {
	sdk::StorageKey key;
	int64_t delta;
};

EXPORT("pub00000000")
call_int64_add()
{
	auto calldata = sdk::get_calldata<calldata_0>();

	sdk::int64_add(calldata.key, calldata.delta);
	return 0;
}

struct calldata_1 {
	sdk::StorageKey key;
	int64_t value;
	int64_t delta;
};

EXPORT("pub01000000")
call_int64_set_add()
{
	auto calldata = sdk::get_calldata<calldata_1>();

	sdk::int64_set_add(calldata.key, calldata.value, calldata.delta);
	return 0;
}
