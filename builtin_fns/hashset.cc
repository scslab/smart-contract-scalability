#include "builtin_fns/builtin_fns.h"

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

void
BuiltinFns::scs_hashset_insert(uint32_t key_offset,
                               /* key_len = 32 */
                               uint32_t hash_offset
                               /* hash_len = 32 */,
                               uint64_t threshold)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    auto hash = runtime.template load_from_memory_to_const_size_buf<Hash>(
        hash_offset);

    tx_ctx.storage_proxy.hashset_insert(
        addr_and_key, hash, threshold, tx_ctx.get_src_tx_hash());
}

void
BuiltinFns::scs_hashset_increase_limit(uint32_t key_offset,
                                       /* key_len = 32 */
                                       uint32_t limit_increase)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    tx_ctx.storage_proxy.hashset_increase_limit(
        addr_and_key, limit_increase, tx_ctx.get_src_tx_hash());
}

void
BuiltinFns::scs_hashset_clear(uint32_t key_offset
                              /* key_len = 32 */,
                              uint64_t threshold)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    tx_ctx.storage_proxy.hashset_clear(
        addr_and_key, threshold, tx_ctx.get_src_tx_hash());
}

const& std::optional<StorageObject>
get_hashset(uint32_t key_offset)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    auto const& res = tx_ctx.storage_proxy.get(addr_and_key);

    if (res.has_value()) {
        if (res->body.type() != ObjectType::HASH_SET) {
            throw wasm_api::HostError("type mismatch in hashset get");
        }
    }

    return res;
}

uint32_t
BuiltinFns::scs_hashset_get_size(uint32_t key_offset
                                 /* key_len = 32 */)
{
    auto const& res = get_hashset(key_offset);

    if (!res) {
        return 0;
    }

    return res->body.hash_set().hashes.size();
}

uint32_t
BuiltinFns::scs_hashset_get_max_size(uint32_t key_offset
                                     /* key_len = 32 */)
{
    auto const& res = get_hashset(key_offset);

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
uint32_t
BuiltinFns::scs_hashset_get_index_of(uint32_t key_offset,
                                     /* key_len = 32 */
                                     uint64_t threshold)
{
    auto const& res = get_hashset(key_offset);

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

void
BuiltinFns::scs_hashset_get_index(uint32_t key_offset,
                                  /* key_len = 32 */
                                  uint32_t output_offset,
                                  /* output_len = 32 */
                                  uint32_t index)
{
    auto const& res = get_hashset(key_offset);

    if (!res) {
        throw wasm_api::HostError("hashset not found");
    }

    auto const& hs = res->body.hash_set().hashes;

    if (hs.hashes.size() <= index) {
        throw wasm_api::HostError("invalid hashset index");
    }

    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    auto& runtime = *tx_ctx.get_current_runtime();

    runtime.write_to_memory(
        hs.hashes[i].hash.data(), output_offset, sizeof(Hash));
}

} // namespace scs
