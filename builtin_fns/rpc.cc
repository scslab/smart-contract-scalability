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

#include "builtin_fns/builtin_fns.h"
#include "builtin_fns/gas_costs.h"

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/global_context.h"
#include "transaction_context/transaction_context.h"

#include "xdr/rpc.h"

#include <utils/time.h>

#include "config.h"

namespace scs {

void
BuiltinFns::scs_external_call(uint32_t target_addr,
                              /* addr_len = 32 */
                              uint32_t call_data_offset,
                              uint32_t call_data_len,
                              uint32_t response_offset,
                              uint32_t response_max_len)
{
    auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();
    auto& tx_ctx = exec_ctx.get_transaction_context();
    auto& runtime = *tx_ctx.get_current_runtime();

    RpcResult result;

    if (tx_ctx.tx_results->is_validating()) {
        result = tx_ctx.tx_results->get_next_rpc_result();
    } else {
        
        #if !USE_RPC
            throw wasm_api::HostError("rpcs disabled in this configuration");
        #else

            auto h = runtime.template load_from_memory_to_const_size_buf<Hash>(
                target_addr);
            auto calldata = runtime.template load_from_memory<xdr::opaque_vec<>>(
                call_data_offset, call_data_len);
    	
            // TODO load balancing, something in background?
            auto sock = exec_ctx.scs_data_structures.address_db.connect(h);
            if (!sock) {
                throw wasm_api::HostError("connection failed");
            }

            ThreadlocalContextStore::get_rate_limiter().free_one_slot();
            auto rpc_result = ThreadlocalContextStore::send_cancellable_rpc(sock, RpcCall(h, calldata));

            if (!rpc_result)
            {
                throw wasm_api::HostError("rpc failed");
            }

            result = *rpc_result;
        #endif
    }

    runtime.write_to_memory(result.result, response_offset, response_max_len);
}

} // namespace scs
