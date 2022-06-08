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
BuiltinFns::scs_raw_memory_set(
	uint32_t key_offset,
	/* key_len = 32 */
	uint32_t mem_offset,
	uint32_t mem_len,
	uint32_t priority)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	auto& runtime = *tx_ctx.get_current_runtime();
	
	auto key = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(key_offset);

	if (mem_len > RAW_MEMORY_MAX_LEN)
	{
		throw wasm_api::HostError("mem write too long");
	}
	auto data = runtime.template load_from_memory<xdr::opaque_vec<RAW_MEMORY_MAX_LEN>>(mem_offset, mem_len);

	auto addr_and_key = tx_ctx.get_storage_key(key);

	tx_ctx.storage_proxy.raw_memory_write(addr_and_key, std::move(data), tx_ctx.get_next_priority(priority));

}

//return 1 if key exists, 0 else
uint32_t
BuiltinFns::scs_raw_memory_get(
	uint32_t key_offset,
	/* key_len = 32 */
	uint32_t output_offset,
	uint32_t output_max_len)
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

	if (res -> type() != ObjectType::RAW_MEMORY)
	{
		throw wasm_api::HostError("type mismatch in raw mem get");
	}

	runtime.write_to_memory(res -> raw_memory_storage().data, output_offset, output_max_len);
	return 1;
}

//returns UINT32_MAX if key nexist
uint32_t
BuiltinFns::scs_raw_memory_get_len(
	uint32_t key_offset
	/* key_len = 32 */)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	auto& runtime = *tx_ctx.get_current_runtime();

	auto key = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(key_offset);

	auto addr_and_key = tx_ctx.get_storage_key(key);

	auto const& res = tx_ctx.storage_proxy.get(addr_and_key);

	if (!res)
	{
		return UINT32_MAX;
	}

	if (res -> type() != ObjectType::RAW_MEMORY)
	{
		throw wasm_api::HostError("type mismatch in raw mem get");
	}

	return res -> raw_memory_storage().data.size();
}



} /* scs */