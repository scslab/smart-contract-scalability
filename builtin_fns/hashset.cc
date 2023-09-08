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

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/method_invocation.h"

#include "wasm_api/error.h"

#include "xdr/storage.h"
#include "xdr/storage_delta.h"

#include <cstdint>
#include <vector>

namespace scs {

BUILTIN_DECL(void)::scs_hashset_insert(uint32_t key_offset,
                               /* key_len = 32 */
                               uint32_t hash_offset
                               /* hash_len = 32 */,
                               uint64_t threshold)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_hashset_insert);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    auto hash = runtime.template load_from_memory_to_const_size_buf<Hash>(
        hash_offset);

    tx_ctx.storage_proxy.hashset_insert(
        addr_and_key, hash, threshold);
}

BUILTIN_DECL(void)::scs_hashset_increase_limit(uint32_t key_offset,
                                       /* key_len = 32 */
                                       uint32_t limit_increase)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_hashset_increase_limit);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    tx_ctx.storage_proxy.hashset_increase_limit(
        addr_and_key, limit_increase);
}

template<typename TransactionContext_t>
std::optional<StorageObject>
get_hashset(TransactionContext_t const& tx_ctx, uint32_t key_offset)
{
    auto const& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    auto res = tx_ctx.storage_proxy.get(addr_and_key);

    if (res.has_value()) {
        if (res->body.type() != ObjectType::HASH_SET) {
            throw wasm_api::HostError("type mismatch in hashset get");
        }
    }

    return res;
}

BUILTIN_DECL(void)::scs_hashset_clear(uint32_t key_offset
                              /* key_len = 32 */,
                              uint64_t threshold)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_hashset_clear);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    tx_ctx.storage_proxy.hashset_clear(
        addr_and_key, threshold);
}

BUILTIN_DECL(uint32_t)::scs_hashset_get_size(uint32_t key_offset
                                 /* key_len = 32 */)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_hashset_get_size);

    auto const& res = get_hashset(tx_ctx, key_offset);

    if (!res) {
        return 0;
    }

    return res->body.hash_set().hashes.size();
}

BUILTIN_DECL(uint32_t)::scs_hashset_get_max_size(uint32_t key_offset
                                     /* key_len = 32 */)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_hashset_get_max_size);

    auto const& res = get_hashset(tx_ctx, key_offset);

    if (!res) {
        return 0;
    }

    return res->body.hash_set().max_size;
}

/**
 *  if two things with same threshold,
 * returns lowest index.
 * If none, returns UINT32_MAX
 **/
BUILTIN_DECL(uint32_t)::scs_hashset_get_index_of(uint32_t key_offset,
                                     /* key_len = 32 */
                                     uint64_t threshold)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_hashset_get_index_of);

    auto const& res = get_hashset(tx_ctx, key_offset);

    if (!res) {
        throw wasm_api::HostError("key nexist (no hashset)");
    }

    auto const& entries = res->body.hash_set().hashes;
    for (auto i = 0u; i < entries.size(); i++) {
        if (entries[i].index == threshold) {
            return i;
        }
    }
    throw wasm_api::HostError("key nexist (not found in hashset)");
}

BUILTIN_DECL(uint64_t)::scs_hashset_get_index(uint32_t key_offset,
                                  /* key_len = 32 */
                                  uint32_t output_offset,
                                  /* output_len = 32 */
                                  uint32_t index)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_hashset_get_index);

    auto const& res = get_hashset(tx_ctx, key_offset);

    if (!res) {
        throw wasm_api::HostError("hashset not found");
    }

    auto const& hs = res->body.hash_set().hashes;

    if (hs.size() <= index) {
        throw wasm_api::HostError("invalid hashset index");
    }

    auto& runtime = *tx_ctx.get_current_runtime();

    runtime.write_to_memory(
        hs[index].hash, output_offset, sizeof(Hash));

    return hs[index].index;
}

} // namespace scs
