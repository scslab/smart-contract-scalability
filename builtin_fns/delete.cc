#include "builtin_fns/builtin_fns.h"
#include "builtin_fns/gas_costs.h"

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"

#include <cstdint>
#include <vector>

namespace scs
{

#if 0
void
BuiltinFns::scs_delete_key_first(
	uint32_t key_offset
	/* key_len = 32 */)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	auto& runtime = *tx_ctx.get_current_runtime();
	
	auto key = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(key_offset);

	auto addr_and_key = tx_ctx.get_storage_key(key);

	tx_ctx.storage_proxy.delete_object_first(addr_and_key, tx_ctx.get_src_tx_hash());
}
#endif

void
BuiltinFns::scs_delete_key_last(
	uint32_t key_offset
	/* key_len = 32 */)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	tx_ctx.consume_gas(gas_delete_key_last);

	auto& runtime = *tx_ctx.get_current_runtime();
	
	auto key = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(key_offset);

	auto addr_and_key = tx_ctx.get_storage_key(key);

	tx_ctx.storage_proxy.delete_object_last(addr_and_key, tx_ctx.get_src_tx_hash());
}

} /* scs */
