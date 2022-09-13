#include "builtin_fns/builtin_fns.h"

#include "threadlocal/threadlocal_context.h"
#include "transaction_context/execution_context.h"

namespace scs
{

int32_t
BuiltinFns::memcmp(uint32_t lhs, uint32_t rhs, uint32_t sz)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();

	auto& runtime = *tx_ctx.get_current_runtime();

	return runtime.memcmp(lhs, rhs, sz);
}

uint32_t
BuiltinFns::memset(uint32_t p, uint32_t val, uint32_t sz)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();

	auto& runtime = *tx_ctx.get_current_runtime();

	return runtime.memset(p, static_cast<uint8_t>(val & 0xFF), sz);
}

uint32_t
BuiltinFns::memcpy(uint32_t dst, uint32_t src, uint32_t sz)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	auto& runtime = *tx_ctx.get_current_runtime();
	return runtime.safe_memcpy(dst, src, sz);
}

} /* scs */
