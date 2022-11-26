#include "builtin_fns/builtin_fns.h"
#include "builtin_fns/gas_costs.h"

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/global_context.h"
#include "transaction_context/transaction_context.h"

#include "xdr/rpc.h"

#include <xdrpp/arpc.h>

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
        auto h = runtime.template load_from_memory_to_const_size_buf<Hash>(
            target_addr);
        auto calldata = runtime.template load_from_memory<xdr::opaque_vec<>>(
            call_data_offset, call_data_len);

        // TODO load balancing, something in background?
        auto sock = exec_ctx.scs_data_structures.address_db.connect(h);
        if (!sock) {
            throw wasm_api::HostError("connection failed");
        }

        auto client = xdr::arpc_client<ContractRPCV1>(*sock);

        uint64_t request_id = ThreadlocalContextStore::get_uid();

        client.execute_call(
            RpcCall(h, calldata), [request_id](xdr::call_result<RpcResult> cb) {
                if (!cb) {
                    ThreadlocalContextStore::timer_notify(std::nullopt,
                                                          request_id);
                } else {
                    ThreadlocalContextStore::timer_notify(*cb, request_id);
                }
            });

        auto result_opt = ThreadlocalContextStore::timer_await(request_id);

        if (!result_opt) {
            throw wasm_api::HostError("no rpc response");
        }
        result = *result_opt;
    }

    runtime.write_to_memory(result.result, response_offset, response_max_len);
}

} // namespace scs
