#pragma once

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

#include <cstdint>

#include "xdr/types.h"

namespace scs
{

Address
compute_contract_deploy_address(
	Address const& deployer_address,
	Hash const& hash,
	uint64_t nonce);

InvariantKey make_static_key(
	uint64_t a,
	uint64_t b = 0,
	uint64_t c = 0,
	uint64_t d = 0);

}