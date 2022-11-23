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
	
void
BuiltinFns::scs_hash(
	uint32_t input_offset,
	uint32_t input_len,
	uint32_t output_offset
	/* output_len = 32 */)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	tx_ctx.consume_gas(gas_hash);

	auto& runtime = *tx_ctx.get_current_runtime();

	auto data = runtime.template load_from_memory<xdr::opaque_vec<MAX_HASH_LEN>>(input_offset, input_len);

	auto s = debug::array_to_str(data);
	EXEC_TRACE("hash input: %s", s.c_str());

	Hash out = hash_xdr(data);

	runtime.write_to_memory(out, output_offset, out.size());
}

uint32_t
BuiltinFns::scs_check_sig_ed25519(
	uint32_t pk_offset,
	/* pk len = 32 */
	uint32_t sig_offset,
	/* sig_len = 64 */
	uint32_t msg_offset,
	uint32_t msg_len)
{
	static_assert(sizeof(PublicKey) == 32, "expected 32 byte pk");
	static_assert(sizeof(Signature) == 64, "expected 64 byte sig");

	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
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
