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

#pragma once

#include "sdk/types.h"

namespace sdk
{

namespace detail
{

BUILTIN("create_contract")
void create_contract(uint32_t contract_idx, uint32_t hash_out);

BUILTIN("deploy_contract")
void deploy_contract(uint32_t hash_offset, uint64_t nonce, uint32_t address_offset_out);

} // namespace detail

Hash create_contract(uint32_t contract_idx)
{
	Hash out;
	detail::create_contract(contract_idx, to_offset(&out));
	return out;
}

Address
deploy_contract(Hash const& h, uint64_t nonce)
{
	Address out;
	detail::deploy_contract(to_offset(&h), nonce, to_offset(&out));
	return out;
}

} // namespace sdk
