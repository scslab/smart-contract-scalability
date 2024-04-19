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
#include "sdk/constexpr.h"
#include "sdk/raw_memory.h"
#include "sdk/invoke.h"
#include "sdk/witness.h"

#include "ed25519.h"

namespace sdk
{

namespace detail
{

constexpr static StorageKey pk_storage_key
	= make_static_key(1);

}


void
wasm_auth_single_pk_register(const PublicKey& pk)
{
	set_raw_memory(detail::pk_storage_key, pk);
}

void 
wasm_auth_single_pk_check_sig(uint64_t wit_idx)
{
	PublicKey pk = get_raw_memory<PublicKey>(detail::pk_storage_key);
	Hash h = get_invoked_hash();
	Signature sig = get_witness<Signature>(wit_idx);

	if (ed25519_verify(sig.data(), h.data(), h.size(), pk.data()) != 1)
	//if (!check_sig_ed25519(pk, sig, h))
	{
		abort();
	}
}


}
