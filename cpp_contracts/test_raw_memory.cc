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
#include "sdk/delete.h"

struct calldata_0 {
	sdk::StorageKey key;
};

EXPORT("pub00000000")
log_key()
{
	auto calldata = sdk::get_calldata<calldata_0>();

	uint64_t stored_value = sdk::get_raw_memory<uint64_t>(calldata.key);

	sdk::log(stored_value);
}

struct calldata_1 {
	sdk::StorageKey key;
	uint64_t value;
};

EXPORT("pub01000000")
set_key()
{
	auto calldata = sdk::get_calldata<calldata_1>();

	sdk::set_raw_memory<uint64_t>(calldata.key, calldata.value);
}

EXPORT("pub02000000")
check_double_write()
{
	auto calldata = sdk::get_calldata<calldata_0>();

	auto const& key = calldata.key;

	calldata_0 c0 {.key = key};
	calldata_1 c1 {.key = key, .value = 0x1234};
	calldata_1 c1_1 {.key = key, .value = 0xAABB};

	auto self = sdk::get_self();

	sdk::invoke(self, 1, c1);
	sdk::invoke(self, 0, c0);
	sdk::invoke(self, 1, c1_1);
	sdk::invoke(self, 0, c0);
}

EXPORT("pub03000000")
check_read_self_writes()
{
	auto calldata = sdk::get_calldata<calldata_0>();

	auto const& key = calldata.key;

	calldata_0 c0 {.key = key};
	calldata_1 c1 {.key = key, .value = 0x1234};

	auto self = sdk::get_self();

	sdk::invoke(self, 1, c1);
	sdk::invoke(self, 0, c0);
}
/*
EXPORT("pub04000000")
delete_key_first()
{
	auto calldata = sdk::get_calldata<calldata_0>();
	auto const& key = calldata.key;
	sdk::delete_first(key);
} */

EXPORT("pub05000000")
delete_key_last()
{
	auto calldata = sdk::get_calldata<calldata_0>();
	auto const& key = calldata.key;
	sdk::delete_last(key);
}
EXPORT("pub06000000")
write_after_delete()
{
	auto calldata = sdk::get_calldata<calldata_0>();
	auto const& key = calldata.key;
	sdk::delete_last(key);

	calldata_1 c1 {.key = key, .value = 0x1234};

	auto self = sdk::get_self();
	sdk::invoke(self, 1, c1);
}
