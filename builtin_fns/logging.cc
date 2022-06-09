#include "builtin_fns/builtin_fns.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/threadlocal_context.h"
#include "transaction_context/method_invocation.h"

#include "debug/debug_macros.h"

#include <cstdint>
#include <vector>

namespace scs
{

void
BuiltinFns::scs_print_debug(uint32_t addr, uint32_t len)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();

	auto& runtime = *tx_ctx.get_current_runtime();

	auto log = runtime.template load_from_memory<std::vector<uint8_t>>(addr, len);

	CONTRACT_INFO("print: addr %lu len %lu value %s", addr, len, debug::array_to_str(log).c_str());
}

void
BuiltinFns::scs_log(
	uint32_t log_offset,
	uint32_t log_len)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();

	auto& runtime = *tx_ctx.get_current_runtime();

	CONTRACT_INFO("Logging offset=%lu len=%lu", log_offset, log_len);

	auto log = runtime.template load_from_memory<std::vector<uint8_t>>(log_offset, log_len);

	tx_ctx.logs.push_back(log);
}


} /* scs */
