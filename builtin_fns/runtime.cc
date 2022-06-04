#include "builtin_fns/builtin_fns.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/threadlocal_context.h"
#include "transaction_context/method_invocation.h"

#include "debug/debug_macros.h"

namespace scs
{

void 
BuiltinFns::scs_return(uint32_t offset, uint32_t len)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();

	tx_ctx.return_buf = tx_ctx.get_current_runtime()->template load_from_memory<std::vector<uint8_t>>(offset, len);
	return;
}

void 
BuiltinFns::scs_get_calldata(uint32_t offset, uint32_t len)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();

	auto& calldata = tx_ctx.get_current_method_invocation().calldata;
	if (static_cast<uint32_t>(len) > calldata.size())
	{
		throw wasm_api::HostError("insufficient calldata");
	}

	tx_ctx.get_current_runtime() -> write_to_memory(calldata, offset, len);
}

uint32_t 
BuiltinFns::scs_invoke(
	uint32_t addr_offset, 
	uint32_t methodname, 
	uint32_t calldata_offset, 
	uint32_t calldata_len,
	uint32_t return_offset,
	uint32_t return_len)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();

	auto& runtime = *tx_ctx.get_current_runtime();

	MethodInvocation invocation(
		runtime.template load_from_memory_to_const_size_buf<Address>(addr_offset),
		methodname,
		runtime.template load_from_memory<std::vector<uint8_t>>(calldata_offset, calldata_len));

	CONTRACT_INFO("call into %s method %lu", debug::array_to_str(invocation.addr).c_str(), methodname);

	ThreadlocalContextStore::get_exec_ctx().invoke_subroutine(invocation);

	if (return_len > 0)
	{
		runtime.write_to_memory(tx_ctx.return_buf, return_offset, return_len);
	}

	tx_ctx.return_buf.clear();

	return return_len;
}

void
BuiltinFns::scs_get_msg_sender(
	uint32_t addr_offset
	/* addr_len = 32 */
	)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	auto& runtime = *tx_ctx.get_current_runtime();

	auto const& invocation = tx_ctx.get_current_method_invocation();

	runtime.write_to_memory(invocation.addr, addr_offset, sizeof(invocation.addr));
}

} /* scs */
