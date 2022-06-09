#include "builtin_fns/builtin_fns.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/threadlocal_context.h"
#include "transaction_context/method_invocation.h"

#include "wasm_api/error.h"

#include "debug/debug_macros.h"

#include "xdr/storage.h"
#include "xdr/storage_delta.h"

#include <cstdint>
#include <vector>

namespace scs
{

void
BuiltinFns::scs_nonnegative_int64_set_add(
		uint32_t key_offset,
		/* key_len = 32 */
		int64_t set_value,
		int64_t delta,
		uint32_t priority)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	auto& runtime = *tx_ctx.get_current_runtime();
	
	auto key = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(key_offset);

	auto addr_and_key = tx_ctx.get_storage_key(key);

	tx_ctx.storage_proxy.nonnegative_int64_set_add(addr_and_key, set_value, delta, tx_ctx.get_next_priority(priority));
}

//return 1 if key exists, 0 else
uint32_t 
BuiltinFns::scs_nonnegative_int64_get(
	uint32_t key_offset,
	/* key_len = 32 */
	uint32_t output_offset
	/* output_len = 8 */)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	auto& runtime = *tx_ctx.get_current_runtime();
	
	auto key = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(key_offset);

	auto addr_and_key = tx_ctx.get_storage_key(key);
	auto const& res = tx_ctx.storage_proxy.get(addr_and_key);

	if (!res)
	{
		return 0;
	}

	if (res -> type() != ObjectType::NONNEGATIVE_INT64)
	{
		throw wasm_api::HostError("type mismatch in raw mem get");
	}

	runtime.write_to_memory(res -> nonnegative_int64(), output_offset);
	return 1;
}

} /* scs */

