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

#include "lib/rsa_fdh.h"

namespace vdf
{

struct vdf_config
{
	sdk::Address rpc_addr;
};

constexpr static sdk::StorageKey config_addr = sdk::make_static_key(0, 2);
constexpr static sdk::StorageKey modulus_addr = sdk::make_static_key(1, 2);
constexpr static sdk::StorageKey init_semaphore_addr = sdk::make_static_key(2, 2);

void 
write_config(vdf_config const& config)
{
	sdk::set_raw_memory(config_addr, config);
}

vdf_config
get_config()
{
	return sdk::get_raw_memory<vdf_config>(config_addr);
}


sdk::Hash
get_vdf(uint64_t query_nonce)
{
	auto config = get_config();

	uint8_t* data_ptr = reinterpret_cast<uint8_t*>(&query_nonce);

	std::vector<uint8_t> query;
	query.insert(query.end(), data_ptr, data_ptr + sizeof(query_nonce));

	auto sig_bytes = sdk::external_call(config.rpc_addr, query, rsa_fdh::RSA_MODULUS_SIZE_BYTES);


	return rsa_fdh::get_vdf_output(rsa_fdh::load_rsa_public_modulus(modulus_addr), sig_bytes, query);
}



struct calldata_init 
{
	sdk::Address rpc_addr;
	std::array<uint8_t, rsa_fdh::RSA_MODULUS_SIZE> modulus;
};


EXPORT("pub00000000")
initialize()
{
	auto calldata = sdk::get_calldata<calldata_init>();

	sdk::TransientSemaphore s(init_semaphore_addr);
	s.acquire();

	write_config(vdf_config {
		.rpc_addr = calldata.rpc_addr
	});

	auto pub_mod = gmp::mpz_from_bytes(calldata.modulus.data(), calldata.modulus.size());

	rsa_fdh::write_rsa_public_modulus(modulus_addr, pub_mod);
}

struct calldata_vdf {
	uint64_t query_nonce;
};

EXPORT("pub01000000")
get_vdf()
{
	auto calldata = sdk::get_calldata<calldata_vdf>();

	auto h = get_vdf(calldata.query_nonce);

	sdk::return_value(h);
}


}