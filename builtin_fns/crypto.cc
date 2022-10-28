#include "builtin_fns/builtin_fns.h"

#include "crypto/hash.h"

#include "debug/debug_utils.h"

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/method_invocation.h"

#include "wasm_api/error.h"

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
	auto& runtime = *tx_ctx.get_current_runtime();

	auto data = runtime.template load_from_memory<xdr::opaque_vec<MAX_HASH_LEN>>(input_offset, input_len);

	Hash out = hash_xdr(data);

	runtime.write_to_memory(out, output_offset, out.size());
}

}