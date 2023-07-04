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

#include "state_db/state_db.h"

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

#include "state_db/modified_keys_list.h"

#include <utils/assert.h>
#include <utils/serialize_endian.h>
#include <utils/time.h>

namespace scs {

void
StateDB::StateDBMetadata::write_to(std::vector<uint8_t>& digest_bytes) const
{
    trie::SnapshotTrieMetadataBase::write_to(digest_bytes);
    utils::append_unsigned_big_endian(digest_bytes, asset_supply);
}

void
StateDB::StateDBMetadata::from_value(
    value_t const& obj)
{
    trie::SnapshotTrieMetadataBase::from_value(obj);
    auto const& committed_obj = obj.get_committed_object();

    utils::print_assert(!!committed_obj,
                        "null obj in statedb! should have been deleted");

    if (committed_obj->body.type() == ObjectType::KNOWN_SUPPLY_ASSET) {
        asset_supply = committed_obj->body.asset().amount;
    }
}

StateDB::StateDBMetadata&
StateDB::StateDBMetadata::operator+=(const StateDBMetadata& other)
{
    trie::SnapshotTrieMetadataBase::operator+=(other);
    asset_supply += other.asset_supply;
    return *this;
}

void
StateDB::assert_not_uncommitted_deltas() const
{
    if (has_uncommitted_deltas) {
        throw std::runtime_error("assert not uncommitted deltas fail");
    }
}

std::optional<StorageObject>
StateDB::get_committed_value(const AddressAndKey& a) const
{
    auto const* res = state_db.get_value(a);
    if (res) {
        return (res)->get_committed_object();
    }
    return std::nullopt;
}

std::optional<RevertableObject::DeltaRewind>
StateDB::try_apply_delta(const AddressAndKey& a, const StorageDelta& delta)
{
    auto* res = state_db.get_value(a);

    has_uncommitted_deltas.store(true, std::memory_order_relaxed);

    if (!res) {
        return new_key_cache.try_reserve_delta(a, delta);
    } else {
        return (res)->try_add_delta(delta);
    }
}

void
merge_impossible(const StateDB::value_t& value,
                 const std::optional<StorageObject>& new_obj)
{
    throw std::runtime_error("cannot merge old value with new");
}

struct UpdateFn
{
    NewKeyCache& new_key_cache;
    StateDB::trie_t& main_db;
    using prefix_t = StateDB::prefix_t;

    template<typename Applyable>
    void operator()(const Applyable& work_root)
    {
        // WARNING -- don't save these pointers -- they might get deleted during
        // the normalization cleanup, and regardless accessing them later
        // doesn't guarantee hash invalidation.
        auto* main_db_subnode = main_db.get_subnode_ref_and_invalidate_hash(
            work_root.get_prefix(), work_root.get_prefix_len());

        if (main_db_subnode->get_prefix_len() != work_root.get_prefix_len()) {
            std::printf("len mismatch got %s wanted %s\n",
                        main_db_subnode->get_prefix()
                            .to_string(main_db_subnode->get_prefix_len())
                            .c_str(),
                        work_root.get_prefix()
                            .to_string(work_root.get_prefix_len())
                            .c_str());
            std::fflush(stdout);
            throw std::runtime_error("wtf");
        }

        auto apply_lambda = [this, main_db_subnode, &work_root](
                                const prefix_t& addrkey,
                                const trie::EmptyValue&) {
            auto* main_db_value = main_db_subnode->get_value(addrkey);
            if (main_db_value) {
                main_db_subnode->invalidate_hash_to_key(addrkey);

                main_db_value->commit_round();

                if (!(main_db_value->get_committed_object())) {
                    main_db_subnode->delete_value(addrkey, main_db.get_gc());
                }
            } else {
                AddressAndKey query
                    = addrkey.template get_bytes_array<AddressAndKey>();

                std::optional<StorageObject> new_obj
                    = new_key_cache.commit_and_get(query);

                if (new_obj) {
                    main_db_subnode->template insert<&merge_impossible>(
                        addrkey, main_db.get_gc(), *new_obj);
                }

                // otherwise, main_db_value is nullptr,
                // so value does not exist.
                // In this case, while the actual trie node may exist,
                // the value will get cleaned up (and the node will be gc'd)
                // during the normalization phase.
                // This enables the deletion of the dead code that was present
                // here (we do not need to delete a value that does not exist,
                // but that was created and needed to be deleted in earlier
                // versions of this code).
            }
        };

        // preallocating reuseable buffer helps performance
        std::vector<uint8_t> digest_bytes;

        work_root.apply_to_kvs(apply_lambda);
        main_db_subnode->compute_hash_and_normalize(main_db.get_gc(),
                                                    digest_bytes);
    }
};

void
StateDB::commit_modifications(const ModifiedKeysList& list)
{
    auto ts = utils::init_time_measurement();

    new_key_cache.finalize_modifications();

    UpdateFn update(new_key_cache, state_db);

    std::printf("parallel modify before %lf\n", utils::measure_time(ts));

    uint32_t grain_size = std::max<uint32_t>(1, list.size() / 100);
    list.get_keys()
        .parallel_batch_value_modify_const<UpdateFn, prefix_t::len()>(
            update, grain_size);

    std::printf("parallel modify after %lf\n", utils::measure_time(ts));

    new_key_cache.clear_for_next_block();

    state_db.hash_and_normalize();

    std::printf("clear and root hash %lf\n", utils::measure_time(ts));

    state_db.do_gc();
    has_uncommitted_deltas = false;
    std::printf("finish %lf\n", utils::measure_time(ts));
}

/**
 * Helper class for rewinding a transaction block that gets rejected.
 * Relies on the fact that values aren't inserted into StateDB before
 * commitment of a block.
 */
struct RewindFn
{
    StateDB::trie_t& main_db;

    using prefix_t = StateDB::prefix_t;

    template<typename Applyable>
    void operator()(const Applyable& work_root)
    {
        auto* main_db_subnode = main_db.get_subnode_ref_and_invalidate_hash(
            work_root.get_prefix(), work_root.get_prefix_len());

        auto apply_lambda = [this, main_db_subnode](const prefix_t& addrkey,
                                                    const trie::EmptyValue&) {
            StateDB::value_t* main_db_value
                = main_db_subnode->get_value(addrkey);
            if (main_db_value) {
                // no need to invalidate hashes when rewinding
                main_db_value->rewind_round();
            }
            // otherwise new key, don't need to do anything
        };

        work_root.apply_to_kvs(apply_lambda);
    }
};

void
StateDB::rewind_modifications(const ModifiedKeysList& list)
{
    new_key_cache.clear_for_next_block();

    RewindFn rewind(state_db);

    list.get_keys()
        .parallel_batch_value_modify_const<RewindFn, prefix_t::len()>(rewind,
                                                                      1);

    state_db.hash_and_normalize();

    has_uncommitted_deltas = false;
}

Hash
StateDB::hash()
{
    assert_not_uncommitted_deltas();

    auto h = state_db.hash_and_normalize();
    Hash out;
    std::memcpy(out.data(), h.data(), h.size());
    return out;
}

} // namespace scs
