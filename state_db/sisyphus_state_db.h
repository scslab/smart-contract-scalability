#pragma once

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

#include "xdr/storage.h"
#include "xdr/types.h"

#include "mtt/common/utils.h"
#include "mtt/snapshot_trie/atomic_merkle_trie.h"

#include <map>
#include <optional>

#include <xdrpp/marshal.h>

#include "object/revertable_object.h"

#include "config/static_constants.h"

namespace scs {

class TypedModificationIndex;

class SisyphusStateDB
{
    static void serialize(std::vector<uint8_t>& buf, const RevertableObject& v)
    {
        auto const& res = v.get_committed_object();
        if (res) {
            xdr::append_xdr_to_opaque(buf, *res);
        }
    }

    static bool validate_value(const RevertableObject& v)
    {
        return (bool)v.get_committed_object();
    }

    template<typename T>
    using value_wrapper_t = trie::OptionalValue<T, &validate_value>;

  public:
    using prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey)>;

    using value_t = trie::BetterSerializeWrapper<RevertableObject, &serialize>;

    struct SisyphusStateMetadata : public trie::SnapshotTrieMetadataBase
    {
        using uint128_t = unsigned __int128;
        uint128_t asset_supply = 0;

        void write_to(std::vector<uint8_t>& digest_bytes) const
        {
            utils::append_unsigned_little_endian(digest_bytes, asset_supply);
            trie::SnapshotTrieMetadataBase::write_to(digest_bytes);
        }
        void from_value(value_t const& obj);
        SisyphusStateMetadata& operator+=(const SisyphusStateMetadata& other);
    };

    using metadata_t = SisyphusStateMetadata;

    using trie_t = trie::AtomicMerkleTrie<prefix_t,
                                          value_t,
                                          TLCACHE_SIZE,
                                          metadata_t,
                                          &validate_value>;

  private:
    trie_t state_db;

  public:
    std::optional<StorageObject> get_committed_value(
        const AddressAndKey& a) const;

    std::optional<RevertableObject::DeltaRewind> try_apply_delta(
        const AddressAndKey& a,
        const StorageDelta& delta);

    void commit_modifications(const TypedModificationIndex& list);

    void rewind_modifications(const TypedModificationIndex& list);

    Hash hash();
};

} // namespace scs
