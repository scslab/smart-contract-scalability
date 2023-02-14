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

#include "wasm_api/error.h"

#include "xdr/storage.h"
#include "xdr/storage_delta.h"

#include <cstdint>
#include <vector>

namespace scs {

void
BuiltinFns::scs_nonnegative_int64_set_add(uint32_t key_offset,
                                          /* key_len = 32 */
                                          int64_t set_value,
                                          int64_t delta)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_nonnegative_int64_set_add);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    tx_ctx.storage_proxy.nonnegative_int64_set_add(
        addr_and_key, set_value, delta, tx_ctx.get_src_tx_hash());
}

void
BuiltinFns::scs_nonnegative_int64_add(uint32_t key_offset,
                                      /* key_len = 32 */
                                      int64_t delta)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_nonnegative_int64_add);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    EXEC_TRACE("int64 add %lld to key %s",
               delta,
               debug::array_to_str(addr_and_key).c_str());

    tx_ctx.storage_proxy.nonnegative_int64_add(
        addr_and_key, delta, tx_ctx.get_src_tx_hash());
}

// return 1 if key exists, 0 else
uint32_t
BuiltinFns::scs_nonnegative_int64_get(uint32_t key_offset,
                                      /* key_len = 32 */
                                      uint32_t output_offset
                                      /* output_len = 8 */)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_nonnegative_int64_get);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);
    auto const& res = tx_ctx.storage_proxy.get(addr_and_key);

    if (!res) {
        return 0;
    }

    if (res->body.type() != ObjectType::NONNEGATIVE_INT64) {
        throw wasm_api::HostError("type mismatch in raw mem get");
    }

    runtime.write_to_memory(res->body.nonnegative_int64(), output_offset);
    return 1;
}

} // namespace scs
