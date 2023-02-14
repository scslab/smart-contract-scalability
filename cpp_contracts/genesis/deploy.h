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
#include "sdk/types.h"

#include "genesis/addresses.h"

namespace deploy
{

struct calldata_DeployAndInitialize
{
	sdk::Hash contract_hash;
	uint64_t nonce;
	uint32_t ctor_method;
	uint32_t ctor_calldata_size;
};

struct calldata_Deploy
{
	sdk::Hash contract_hash;
	uint64_t nonce;
};

struct calldata_Create
{
	uint32_t contract_idx;
};

[[maybe_unused]]
static void
deploy_and_initialize(
	sdk::Hash const& h, 
	uint64_t nonce,
	uint32_t ctor_method,
	uint8_t const* ctor_calldata,
	const uint32_t ctor_calldata_len)
{
	uint8_t calldata[sizeof(calldata_DeployAndInitialize) + ctor_calldata_len];

	calldata_DeployAndInitialize& c = *reinterpret_cast<calldata_DeployAndInitialize*>(calldata);

	c.contract_hash = h;
	c.nonce = nonce;
	c.ctor_method = ctor_method;
	c.ctor_calldata_size = ctor_calldata_len;

	std::memcpy(calldata + sizeof(calldata_DeployAndInitialize), ctor_calldata, ctor_calldata_len);

	sdk::invoke(genesis::DEPLOY_ADDRESS, 0, calldata, sizeof(calldata_DeployAndInitialize) + ctor_calldata_len);
}

}