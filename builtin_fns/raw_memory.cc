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

BUILTIN_DECL(void)::scs_raw_memory_set(uint32_t key_offset,
                               /* key_len = 32 */
                               uint32_t mem_offset,
                               uint32_t mem_len)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_raw_memory_set(mem_len));

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    if (mem_len > RAW_MEMORY_MAX_LEN) {
        throw wasm_api::HostError("mem write too long");
    }
    auto data
        = runtime
              .template load_from_memory<xdr::opaque_vec<RAW_MEMORY_MAX_LEN>>(
                  mem_offset, mem_len);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    tx_ctx.storage_proxy.raw_memory_write(
        addr_and_key, std::move(data));
}

// return 1 if key exists, 0 else
BUILTIN_DECL(uint32_t)::scs_raw_memory_get(uint32_t key_offset,
                               /* key_len = 32 */
                               uint32_t output_offset,
                               uint32_t output_max_len)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_raw_memory_get(output_max_len));

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    EXEC_TRACE("raw_memory get %s", debug::array_to_str(addr_and_key).c_str());

    auto const& res = tx_ctx.storage_proxy.get(addr_and_key);

    if (!res) {
        return 0;
    }

    if (res->body.type() != ObjectType::RAW_MEMORY) {
        throw wasm_api::HostError("type mismatch in raw mem get");
    }

    runtime.write_to_memory(
        res->body.raw_memory_storage().data, output_offset, output_max_len);
    return 1;
}

// returns UINT32_MAX if key nexist
BUILTIN_DECL(uint32_t)::scs_raw_memory_get_len(uint32_t key_offset
                                   /* key_len = 32 */)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_raw_memory_get_len);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    auto const& res = tx_ctx.storage_proxy.get(addr_and_key);

    if (!res) {
        return UINT32_MAX;
    }

    if (res->body.type() != ObjectType::RAW_MEMORY) {
        throw wasm_api::HostError("type mismatch in raw mem get");
    }

    return res->body.raw_memory_storage().data.size();
}

} // namespace scs
