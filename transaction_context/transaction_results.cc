#include "transaction_context/transaction_results.h"

#include <wasm_api/error.h>

namespace scs
{

using xdr::operator==;


void
TransactionResultsFrame::add_log(TransactionLog log)
{
	if (!validating)
	{
		results.logs.push_back(log);
	} 
	else
	{
		if (results.logs.size() <= log_idx)
		{
			throw wasm_api::HostError("mismatch log counts");
		}
		if (results.logs[log_idx] != log)
		{
			throw wasm_api::HostError("mismatch in logs");
		}
	}
}

void
TransactionResultsFrame::add_rpc_result(RpcResult result)
{
	if (!validating)
	{
		results.results.push_back(result);
	} 
	else
	{
		throw wasm_api::HostError("added rpc result when validating");
	}
}

RpcResult
TransactionResultsFrame::get_next_rpc_result()
{
	if (rpc_idx < results.results.size())
	{
		auto const& out = results.results[rpc_idx];
		rpc_idx ++;
		return out;
	} 
	else
	{
		throw wasm_api::HostError("insufficient rpc results stored");
	}
}

}
