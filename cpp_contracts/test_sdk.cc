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

#include "sdk/replay_cache.h"
#include "sdk/semaphore.h"
#include "sdk/constexpr.h"
#include "sdk/calldata.h"

struct replay_calldata {
	uint64_t expiry_time;
};

EXPORT("pub00000000")
replay()
{
	auto calldata = sdk::get_calldata<replay_calldata>();
	sdk::record_self_replay(calldata.expiry_time);
	return 0;
}

static sdk::Semaphore s(sdk::make_static_key(0, 1));
static sdk::Semaphore<2> s2 (sdk::make_static_key(1, 1));

EXPORT("pub01000000")
semaphore()
{
	s.acquire();
	return 0;
}

EXPORT("pub02000000")
semaphore2()
{
	s2.acquire();
	return 0;
}

EXPORT("pub03000000")
transientSemaphore()
{
	sdk::TransientSemaphore (sdk::make_static_key(2, 1)).acquire();
	return 0;
}

