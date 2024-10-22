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

#include "sdk/log.h"
#include "sdk/calldata.h"
#include "sdk/invoke.h"

EXPORT("pub00000000")
log_value()
{
	uint64_t val = 0xAABBCCDD'EEFF0011;

	sdk::log(val);
	return 0;
}

EXPORT("pub01000000")
log_calldata()
{
	uint64_t calldata = sdk::get_calldata<uint64_t>();
	sdk::log(calldata);
	return 0;
}

EXPORT("pub02000000")
log_data_twice()
{
	struct calldata_t {
		uint32_t a;
		uint32_t b;
	};

	auto calldata = sdk::get_calldata<calldata_t>();
	sdk::log(calldata.a);
	sdk::log(calldata.b);
	return 0;
}

EXPORT("pub03000000")
log_msg_sender()
{
	auto addr = sdk::get_msg_sender();
	sdk::log(addr);
	return 0;
}

EXPORT("pub04000000")
log_msg_sender_from_self()
{
	sdk::invoke(sdk::get_self(), 3, sdk::EmptyStruct{});
	return 0;
}
