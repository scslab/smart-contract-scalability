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

#include <cstdint>
#include <vector>

namespace scs {

void
BuiltinFns::scs_asset_add(uint32_t key_offset,
                          /* key_len = 32 */
                          int64_t delta)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_asset_add);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    tx_ctx.storage_proxy.asset_add(addr_and_key, delta);
}

uint64_t
BuiltinFns::scs_asset_get(uint32_t key_offset
                          /* key_len = 32 */)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_asset_get);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);
    auto const& res = tx_ctx.storage_proxy.get(addr_and_key);

    if (!res) {
        return 0;
    }

    if (res->body.type() != ObjectType::KNOWN_SUPPLY_ASSET) {
        throw wasm_api::HostError("type mismatch in asset get");
    }

    return res -> body.asset().amount;
}

} // namespace scs
