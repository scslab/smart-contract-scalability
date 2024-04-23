/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "transaction_context/transaction_results.h"

#include "transaction_context/error.h"

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
		throw HostError("added rpc result when validating");
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
		throw HostError("insufficient rpc results stored");
	}
}

bool
TransactionResultsFrame::validating_check_all_rpc_results_used() const
{
	if (!validating) {
		return true;
	}

	return (rpc_idx == results.ndeterministic_results.rpc_results.size());
}


}
