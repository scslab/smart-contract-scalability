#include "transaction_context/transaction_results.h"

#include <wasm_api/error.h>

namespace scs
{

using xdr::operator==;


void
TransactionResultsFrame::add_log(TransactionLog log)
{
	results.logs.push_back(log);
}

void
TransactionResultsFrame::add_rpc_result(RpcResult result)
{
	if (!validating)
	{
		results.ndeterministic_results.rpc_results.push_back(result);
	} 
	else
	{
		throw wasm_api::HostError("added rpc result when validating");
	}
}

RpcResult
TransactionResultsFrame::get_next_rpc_result()
{
	if (rpc_idx < results.ndeterministic_results.rpc_results.size())
	{
		auto const& out = results.ndeterministic_results.rpc_results[rpc_idx];
		rpc_idx ++;
		return out;
	} 
	else
	{
		throw wasm_api::HostError("insufficient rpc results stored");
	}
}

}
