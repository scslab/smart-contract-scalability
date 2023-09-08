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

#include "config/static_constants.h"

namespace scs {

class ModifiedKeysList;

class StateDBv2
{
    static std::vector<uint8_t> serialize(const RevertableObject& v)
    {
        auto const& res = v.get_committed_object();
        if (res) {
            return xdr::xdr_to_opaque(*res);
        }
        return {};
    }

    static bool validate_value(const RevertableObject& v)
    {
        return (bool)v.get_committed_object();
    }

    template<typename T>
    using value_wrapper_t = trie::OptionalValue<T, &validate_value>;

  public:
    using prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey)>;

    using value_t = trie::SerializeWrapper<RevertableObject, &serialize>;

    using metadata_t = trie::SnapshotTrieMetadataBase;

    using trie_t = trie::AtomicMerkleTrie<prefix_t, value_t, TLCACHE_SIZE, metadata_t, value_wrapper_t>;

  private:
    trie_t state_db;

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
