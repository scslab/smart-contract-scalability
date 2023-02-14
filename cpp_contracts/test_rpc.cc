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
#include "sdk/calldata.h"
#include "sdk/crypto.h"
#include "sdk/invoke.h"
#include "sdk/log.h"
#include "sdk/nonnegative_int64.h"
#include "sdk/raw_memory.h"
#include "sdk/types.h"
#include "sdk/constexpr.h"
#include "sdk/rpc.h"
#include "sdk/semaphore.h"

uint64_t dispatch_call(sdk::Hash const& addr, uint64_t input)
{
	return sdk::external_call<uint64_t, uint64_t>(addr, input);
}

struct calldata {
	sdk::Hash addr;
	uint64_t call;
};

EXPORT("pub00000000")
dispatch()
{
	auto c = sdk::get_calldata<calldata>();

	uint64_t res = dispatch_call(c.addr, c.call);

	if (res != c.call) {
		abort();
	}

	sdk::log(res);
}

