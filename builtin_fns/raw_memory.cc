#include "builtin_fns/builtin_fns.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/threadlocal_context.h"
#include "transaction_context/method_invocation.h"

#include "debug/debug_macros.h"

#include <cstdint>
#include <vector>

namespace scs
{

static void
BuiltinFns::scs_raw_memory_set(
	int32_t key_offset,
	int32_t key_len,
	int32_t mem_offset,
	int32_t mem_len)
{

}

static void
BuiltinFns::scs_raw_memory_get(
	int32_t key_offset,
	int32_t key_len,
	int32_t output_offset,
	int32_t output_max_len)
{

}

static int32_t
BuiltinFns::scs_raw_memory_get_len(
	int32_t key_offset,
	int32_t key_len)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	auto& runtime = *tx_ctx.get_current_runtime();
	auto key = runtime.template load_from_memory<std::vector<uint8_t>>(key_offset, key_len);

	auto& storage_cache = tx_ctx.get_storage_cache();

	auto const& obj = storage_cache.get(key);
}



} /* scs */
