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

#include "mtt/snapshot_trie/atomic_merkle_trie.h"
#include "mtt/common/utils.h"

#include <map>
#include <optional>

#include <xdrpp/marshal.h>

#include "object/revertable_object.h"

#include "state_db/new_key_cache.h"
#include "config/static_constants.h"

namespace scs {

class ModifiedKeysList;

class StateDB
{
    static void serialize(std::vector<uint8_t>& buf, const RevertableObject& v)
    {
        auto const& res = v.get_committed_object();
        if (res) {
            xdr::append_xdr_to_opaque(buf, *res);
        }
    }

  public:
    using prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey)>;

    using value_t = trie::BetterSerializeWrapper<RevertableObject, &serialize>;

    struct StateDBMetadata : public trie::SnapshotTrieMetadataBase
    {
        using uint128_t = unsigned __int128;
       // uint128_t asset_supply = 0;

        void write_to(std::vector<uint8_t>& digest_bytes) const
        {
            trie::SnapshotTrieMetadataBase::write_to(digest_bytes);
        }
        void from_value(value_t const& obj);
        StateDBMetadata& operator+=(const StateDBMetadata& other);
    };

    using metadata_t = StateDBMetadata;

    using trie_t = trie::AtomicMerkleTrie<prefix_t, value_t, TLCACHE_SIZE, metadata_t>;

  private:
    trie_t state_db;
    NewKeyCache new_key_cache;

    std::atomic<bool> has_uncommitted_deltas = false;

    void assert_not_uncommitted_deltas() const;

  public:
    std::optional<StorageObject> get_committed_value(
        const AddressAndKey& a) const;

    std::optional<RevertableObject::DeltaRewind> try_apply_delta(
        const AddressAndKey& a,
        const StorageDelta& delta);

    void commit_modifications(const ModifiedKeysList& list);

    void rewind_modifications(const ModifiedKeysList& list);

    Hash hash();
};

} // namespace scs
