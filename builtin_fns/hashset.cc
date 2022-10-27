#include "builtin_fns/builtin_fns.h"

#include "debug/debug_macros.h"

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
BuiltinFns::scs_hashset_insert(
	uint32_t key_offset,
	/* key_len = 32 */
	uint32_t hash_offset
	/* hash_len = 32 */)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	auto& runtime = *tx_ctx.get_current_runtime();
	
	auto key = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(key_offset);

	auto addr_and_key = tx_ctx.get_storage_key(key);

	auto hash = runtime.template load_from_memory_to_const_size_buf<Hash>(hash_offset);

	tx_ctx.storage_proxy.hashset_insert(addr_and_key, hash, tx_ctx.get_src_tx_hash());
}

void
BuiltinFns::scs_hashset_increase_limit(
	uint32_t key_offset,
	/* key_len = 32 */
	uint32_t limit_increase)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	auto& runtime = *tx_ctx.get_current_runtime();
	
	auto key = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(key_offset);

	auto addr_and_key = tx_ctx.get_storage_key(key);

	tx_ctx.storage_proxy.hashset_increase_limit(addr_and_key, limit_increase, tx_ctx.get_src_tx_hash());
}

void
BuiltinFns::scs_hashset_clear(
	uint32_t key_offset
	/* key_len = 32 */)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	auto& runtime = *tx_ctx.get_current_runtime();
	
	auto key = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(key_offset);

	auto addr_and_key = tx_ctx.get_storage_key(key);

	tx_ctx.storage_proxy.hashset_clear(addr_and_key, tx_ctx.get_src_tx_hash());
}

} // namespace scs
