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
#include "sdk/syscall.h"

namespace sdk
{

Hash create_contract(uint32_t contract_idx)
{
	Hash out;
	detail::builtin_syscall(SYSCALLS::CONTRACT_CREATE,
		contract_idx, to_offset(&out),
		0, 0, 0, 0);
	return out;
}

Address
deploy_contract(Hash const& h, uint64_t nonce)
{
	Address out;
	detail::builtin_syscall(SYSCALLS::CONTRACT_DEPLOY,
		to_offset(&h), nonce, to_offset(&out),
		0, 0, 0);
	return out;
}

} // namespace sdk
