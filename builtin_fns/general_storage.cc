#include "builtin_fns/builtin_fns.h"

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/method_invocation.h"

#include "wasm_api/error.h"

#include "xdr/storage.h"

#include <cstdint>
#include <vector>

namespace scs
{

uint32_t
BuiltinFns::scs_has_key(
	uint32_t key_offset
	/* key_len = 32 */)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	auto& runtime = *tx_ctx.get_current_runtime();
	
	auto key = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(key_offset);

	auto addr_and_key = tx_ctx.get_storage_key(key);

	auto res = tx_ctx.storage_proxy.get(addr_and_key);

	return res.has_value();
}

}
