#include "builtin_fns/builtin_fns.h"

#include "builtin_fns/gas_costs.h"

#include "threadlocal/threadlocal_context.h"
#include "transaction_context/execution_context.h"

namespace scs
{

int32_t
BuiltinFns::memcmp(uint32_t lhs, uint32_t rhs, uint32_t sz)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();

	tx_ctx.consume_gas(gas_memcmp(sz));

	auto& runtime = *tx_ctx.get_current_runtime();

	return runtime.memcmp(lhs, rhs, sz);
}

uint32_t
BuiltinFns::memset(uint32_t p, uint32_t val, uint32_t sz)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	tx_ctx.consume_gas(gas_memset(sz));

	auto& runtime = *tx_ctx.get_current_runtime();

	return runtime.memset(p, static_cast<uint8_t>(val & 0xFF), sz);
}

uint32_t
BuiltinFns::memcpy(uint32_t dst, uint32_t src, uint32_t sz)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	tx_ctx.consume_gas(gas_memcpy(sz));

	auto& runtime = *tx_ctx.get_current_runtime();
	return runtime.safe_memcpy(dst, src, sz);
}

uint32_t
BuiltinFns::strnlen(uint32_t str, uint32_t max_len)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	tx_ctx.consume_gas(gas_strnlen(max_len));
	
	auto& runtime = *tx_ctx.get_current_runtime();
	return runtime.safe_strlen(str, max_len);
}

} /* scs */
