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
#include "mtt/memcached_snapshot_trie/serialize_disk_interface.h"
#include "mtt/memcached_snapshot_trie/memcache_trie.h"
#include "mtt/memcached_snapshot_trie/durable_interface.h"
#include "mtt/memcached_snapshot_trie/null_interface.h"

#include "state_db/async_keys_to_disk.h"

#include <map>
#include <optional>

#include <xdrpp/marshal.h>

#include "object/revertable_object.h"

#include "config/static_constants.h"

namespace scs {

class TypedModificationIndex;

class SisyphusValueWrapper : public RevertableObject
{
    using slice_t = trie::DurableValueSlice;

    static StorageObject from_slice(const slice_t& slice) {
        throw std::runtime_error("unimplemented");
    }

public:

    using RevertableObject::RevertableObject;

    SisyphusValueWrapper(slice_t const& slice) : RevertableObject(from_slice(slice)) {}

    void copy_data(std::vector<uint8_t>& buf) const
    {
        auto const& res = get_committed_object();
        if (res) {
            xdr::append_xdr_to_opaque(buf, *res);
        }
    }
};

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

    //template<typevalue_wrapper_tname T>
    //using  = trie::OptionalValue<T, &validate_value>;

  public:
    using prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey)>;

    using value_t = SisyphusValueWrapper; // trie::BetterSerializeWrapper<RevertableObject, &serialize>;

/*
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
    */

    using metadata_t = trie::SnapshotTrieMetadataBase;
    using null_storage_t = trie::NullInterface<sizeof(AddressAndKey)>;
    using nonnull_storage_t = trie::SerializeDiskInterface<sizeof(AddressAndKey), TLCACHE_SIZE>;
    using storage_t = typename std::conditional<PERSISTENT_STORAGE_ENABLED, nonnull_storage_t, null_storage_t>::type;

    using trie_t = trie::MemcacheTrie<prefix_t,
                                          value_t,
                                          TLCACHE_SIZE,
                                          storage_t,
                                          metadata_t,
                                          &validate_value>;

  private:
    uint32_t current_timestamp = 0;

    trie_t state_db;

  public:

    SisyphusStateDB() 
        : current_timestamp(0)
        , state_db(current_timestamp)
        {}

    std::optional<StorageObject> get_committed_value(
        const AddressAndKey& a);

    std::optional<RevertableObject::DeltaRewind> try_apply_delta(
        const AddressAndKey& a,
        const StorageDelta& delta);

    void commit_modifications(const TypedModificationIndex& list);

    void rewind_modifications(const TypedModificationIndex& list);

    Hash hash();

    void set_timestamp(uint32_t ts) {
        current_timestamp = ts;
    }

    void log_keys(AsyncKeysToDisk& logger)
    {
        logger.log_keys(state_db.get_storage(), current_timestamp);
    }
};

} // namespace scs
