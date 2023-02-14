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

#include "sdk/invoke.h"
#include "sdk/calldata.h"
#include "sdk/contracts.h"
#include "sdk/types.h"

#include "genesis/deploy.h"

namespace deploy
{


EXPORT("pub00000000")
deployAndInitialize()
{
	auto calldata_fixed = sdk::get_calldata<calldata_DeployAndInitialize>();

	uint8_t invoke_data[calldata_fixed.ctor_calldata_size];

	sdk::get_calldata_slice(
		invoke_data,
		sizeof(calldata_DeployAndInitialize),
		sizeof(calldata_DeployAndInitialize) + calldata_fixed.ctor_calldata_size);


	auto deployed_addr = sdk::deploy_contract(calldata_fixed.contract_hash, calldata_fixed.nonce);

	sdk::invoke(deployed_addr, calldata_fixed.ctor_method, invoke_data, calldata_fixed.ctor_calldata_size);
}

EXPORT("pub01000000")
deploy()
{
	auto calldata = sdk::get_calldata<calldata_Deploy>();

	sdk::deploy_contract(calldata.contract_hash, calldata.nonce);
}

EXPORT("pub02000000")
create()
{
	auto calldata = sdk::get_calldata<calldata_Create>();

	sdk::create_contract(calldata.contract_idx);
}


}