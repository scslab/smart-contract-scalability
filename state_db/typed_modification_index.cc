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

#include "state_db/typed_modification_index.h"

#include <utils/serialize_endian.h>

#include "crypto/hash.h"

namespace scs {

template<size_t enum_count>
constexpr bool
validate_enum(std::array<int32_t, enum_count> const& vals)
{
    for (auto const& v : vals) {
        if ((v < 0) || (v >= 256)) {
            return false;
        }
    }
    return true;
}

TypedModificationIndex::trie_prefix_t
make_index_key(AddressAndKey const& addrkey,
               StorageDelta const& delta,
               Hash const& src_tx_hash)
{
    TypedModificationIndex::trie_prefix_t out;
    // relying on ctor of trie_prefix_t to zero out memory

    auto* data = out.underlying_data_ptr();

    std::memcpy(data, addrkey.data(), sizeof(AddressAndKey));

    data += sizeof(AddressAndKey);

    static_assert(xdr::xdr_traits<DeltaType>::enum_values.size() < 256,
                  "too many enum vals");
    static_assert(validate_enum(xdr::xdr_traits<DeltaType>::enum_values),
                  "invalid enum vals");

    // conversion int32_t -> uint8_t safe because of check above
    uint8_t enum_val = static_cast<int32_t>(delta.type());

    *data = enum_val;
    data++;

    switch (delta.type()) {
        case DeltaType::DELETE_LAST:
        {
            // nothing to do
            break;
        }
        case DeltaType::RAW_MEMORY_WRITE:
        {
            hash_raw(delta.data().data(), delta.data().size(), data);
            break;
        }
        case DeltaType::NONNEGATIVE_INT64_SET_ADD:
        {
            auto const& sa = delta.set_add_nonnegative_int64();
            // apparently standard says this conversion is well-defined
            utils::write_unsigned_big_endian(
                data, static_cast<uint64_t>(sa.set_value));
            utils::write_unsigned_big_endian(data + sizeof(uint64_t),
                                             static_cast<uint64_t>(sa.delta));
            break;
        }
        case DeltaType::HASH_SET_INSERT:
        {
            std::memcpy(
                data, delta.hash().hash.data(), delta.hash().hash.size());
            utils::write_unsigned_big_endian(data + delta.hash().hash.size(),
                                             delta.hash().index);
            break;
        }
        case DeltaType::HASH_SET_INCREASE_LIMIT:
        {
            utils::write_unsigned_big_endian(data, delta.limit_increase());
            break;
        }
        case DeltaType::HASH_SET_CLEAR:
        {
            utils::write_unsigned_big_endian(data, delta.threshold());
            break;
        }
        case DeltaType::ASSET_OBJECT_ADD:
        {
            utils::write_unsigned_big_endian(
                data, static_cast<uint64_t>(delta.asset_delta()));
            break;
        }
        default:
            throw std::runtime_error("unimplemented");
    }
    data += TypedModificationIndex::modification_key_length;
    std::memcpy(data, src_tx_hash.data(), sizeof(Hash));
    return out;
}

void 
TypedModificationIndex::log_modification(
    AddressAndKey const& addrkey, StorageDelta const& mod, Hash const& src_tx_hash)
{
    auto& local_trie = cache.get(keys);
    auto key = make_index_key(addrkey, value_t(mod), src_tx_hash);

    local_trie.insert(key, mod);
}

void 
TypedModificationIndex::save_modifications(ModIndexLog& out)
{
    auto get_fn = [](trie_prefix_t const& prefix, StorageDelta const& delta) -> IndexedModification
    {
        IndexedModification out;
        std::memcpy(out.addr.data(), prefix.underlying_data_ptr(), sizeof(AddressAndKey));
        std::memcpy(out.tx_hash.data(), prefix.underlying_data_ptr() + trie_prefix_t::size_bytes() - sizeof(Hash), sizeof(Hash));
        out.delta = delta;
        return out;
    };

    keys.accumulate_values_parallel<decltype(out), get_fn>(out, 10);
}


Hash 
TypedModificationIndex::hash()
{
    Hash out;
    auto h = keys.hash();
    std::memcpy(out.data(), h.data(), h.size());
    return out;
}

void 
TypedModificationIndex::clear()
{
    cache.clear();
    keys.clear();
}

} // namespace scs
