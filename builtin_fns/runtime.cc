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

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/method_invocation.h"

namespace scs {

void
BuiltinFns::scs_return(uint32_t offset, uint32_t len)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_return(len));

    tx_ctx.return_buf
        = tx_ctx.get_current_runtime()
              ->template load_from_memory<std::vector<uint8_t>>(offset, len);
    return;
}

void
BuiltinFns::scs_get_calldata(uint32_t offset,
                             uint32_t calldata_slice_start,
                             uint32_t calldata_slice_end)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();

    auto& calldata = tx_ctx.get_current_method_invocation().calldata;
    if (calldata_slice_end > calldata.size()) {
        throw wasm_api::HostError("insufficient calldata");
    }

    if (calldata_slice_end < calldata_slice_start) {
        throw wasm_api::HostError("invalid calldata params");
    }

    tx_ctx.consume_gas(
        gas_get_calldata(calldata_slice_end - calldata_slice_start));

    tx_ctx.get_current_runtime()->write_slice_to_memory(
        calldata, offset, calldata_slice_start, calldata_slice_end);
}

uint32_t
BuiltinFns::scs_get_calldata_len()
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_get_calldata_len);

    auto& calldata = tx_ctx.get_current_method_invocation().calldata;
    return calldata.size();
}

uint32_t
BuiltinFns::scs_invoke(uint32_t addr_offset,
                       uint32_t methodname,
                       uint32_t calldata_offset,
                       uint32_t calldata_len,
                       uint32_t return_offset,
                       uint32_t return_len)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_invoke(calldata_len + return_len));

    auto& runtime = *tx_ctx.get_current_runtime();

    MethodInvocation invocation(
        runtime.template load_from_memory_to_const_size_buf<Address>(
            addr_offset),
        methodname,
        runtime.template load_from_memory<std::vector<uint8_t>>(calldata_offset,
                                                                calldata_len));

    CONTRACT_INFO("call into %s method %lu",
                  debug::array_to_str(invocation.addr).c_str(),
                  methodname);

    ThreadlocalContextStore::get_exec_ctx().invoke_subroutine(invocation);

    if (return_len > 0) {
        runtime.write_to_memory(tx_ctx.return_buf, return_offset, return_len);
    }

    tx_ctx.return_buf.clear();

    return return_len;
}

void
BuiltinFns::scs_get_msg_sender(uint32_t addr_offset
                               /* addr_len = 32 */
)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_get_msg_sender);

    auto& runtime = *tx_ctx.get_current_runtime();

    Address const& sender = tx_ctx.get_msg_sender();

    runtime.write_to_memory(sender, addr_offset, sizeof(Address));
}

void
BuiltinFns::scs_get_self_addr(uint32_t addr_offset
                              /* addr_len = 32 */
)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_get_self_addr);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto const& invoke = tx_ctx.get_current_method_invocation();

    runtime.write_to_memory(invoke.addr, addr_offset, sizeof(Address));
}

uint64_t
BuiltinFns::scs_get_block_number()
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_get_block_number);

    return tx_ctx.get_block_number();
}

void
BuiltinFns::scs_get_src_tx_hash(uint32_t hash_offset)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_get_src_tx_hash);

    auto& runtime = *tx_ctx.get_current_runtime();

    Hash const& h = tx_ctx.get_src_tx_hash();

    runtime.write_to_memory(h, hash_offset, sizeof(Hash));
}

void
BuiltinFns::scs_get_invoked_tx_hash(uint32_t hash_offset)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_get_invoked_tx_hash);

    auto& runtime = *tx_ctx.get_current_runtime();

    Hash const& h = tx_ctx.get_invoked_tx_hash();

    runtime.write_to_memory(h, hash_offset, sizeof(Hash));
}

void
BuiltinFns::scs_gas(uint64_t consumed_gas)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(consumed_gas);
}

} // namespace scs
