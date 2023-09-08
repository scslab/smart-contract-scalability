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

#include "builtin_fns/builtin_fns.h"
#include "builtin_fns/gas_costs.h"

#include "crypto/hash.h"
#include "crypto/crypto_utils.h"

#include "debug/debug_utils.h"
#include "debug/debug_macros.h"

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/method_invocation.h"

#include "wasm_api/error.h"

#include "xdr/types.h"
#include "xdr/storage.h"
#include "xdr/storage_delta.h"

#include <cstdint>
#include <vector>


namespace scs
{
	
BUILTIN_DECL(void)::scs_hash(
	uint32_t input_offset,
	uint32_t input_len,
	uint32_t output_offset
	/* output_len = 32 */)
{
	auto& tx_ctx = GET_TEC;
	tx_ctx.consume_gas(gas_hash);

	auto& runtime = *tx_ctx.get_current_runtime();

	auto data = runtime.template load_from_memory<xdr::opaque_vec<MAX_HASH_LEN>>(input_offset, input_len);

	auto s = debug::array_to_str(data);
	EXEC_TRACE("hash input: %s", s.c_str());

	Hash out = hash_xdr(data);

	runtime.write_to_memory(out, output_offset, out.size());
}

BUILTIN_DECL(uint32_t)::scs_check_sig_ed25519(
	uint32_t pk_offset,
	/* pk len = 32 */
	uint32_t sig_offset,
	/* sig_len = 64 */
	uint32_t msg_offset,
	uint32_t msg_len)
{
	static_assert(sizeof(PublicKey) == 32, "expected 32 byte pk");
	static_assert(sizeof(Signature) == 64, "expected 64 byte sig");

	auto& tx_ctx = GET_TEC;
	tx_ctx.consume_gas(gas_check_sig_ed25519);
	
	auto& runtime = *tx_ctx.get_current_runtime();

	auto pk = runtime.template load_from_memory_to_const_size_buf<PublicKey>(pk_offset);
	auto sig = runtime.template load_from_memory_to_const_size_buf<Signature>(sig_offset);

	auto msg = runtime.template load_from_memory<std::vector<uint8_t>>(msg_offset, msg_len);

	bool res = check_sig_ed25519(pk, sig, msg);

	auto s = debug::array_to_str(msg);
	EXEC_TRACE("sig check: %lu on msg %s", res, s.c_str());

	return res;
}

}
