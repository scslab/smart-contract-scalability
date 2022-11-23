#include "builtin_fns/builtin_fns.h"
#include "builtin_fns/gas_costs.h"

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"


namespace scs
{

void
BuiltinFns::scs_get_witness(uint64_t wit_idx, uint32_t out_offset, uint32_t max_len)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	tx_ctx.consume_gas(gas_get_witness(max_len));

	auto& runtime = *tx_ctx.get_current_runtime();

	auto const& wit = tx_ctx.get_witness(wit_idx);

	runtime.write_to_memory(wit.value, out_offset, max_len);
}

uint32_t
BuiltinFns::scs_get_witness_len(uint64_t wit_idx)
{
	auto& tx_ctx = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
	tx_ctx.consume_gas(gas_get_witness_len);

	return tx_ctx.get_witness(wit_idx).value.size();
}


}