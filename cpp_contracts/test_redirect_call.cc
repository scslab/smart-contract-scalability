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
#include "sdk/invoke.h"
#include "sdk/alloc.h"

static uint32_t flag = 0;

EXPORT("pub00000000")
redirect_call_to_proxy()
{
	struct calldata_t {
		sdk::Address callee;
		uint32_t method;
	};

	auto calldata = sdk::get_calldata<calldata_t>();

	sdk::print_debug(calldata);

	flag = 1;
	sdk::invoke(calldata.callee, calldata.method, sdk::EmptyStruct{});
	flag = 0;
}

EXPORT("pub01000000")
self_call()
{
	sdk::log((uint8_t)0xFF);
}

EXPORT("pub02000000")
self_call_reentrant_guard()
{
	if (flag) {
		abort();
	}
}
