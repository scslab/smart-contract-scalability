#include "state_db/sisyphus_state_db.h"
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
#include <type_traits>

#include <utils/time.h>
#include "state_db/typed_modification_index.h"

namespace scs {

/*
void
SisyphusStateDB::SisyphusStateMetadata::from_value(value_t const& obj)
{
    auto const& res = obj.get_committed_object();
    if (res) {
        if (res->body.type() == ObjectType::KNOWN_SUPPLY_ASSET) {
            asset_supply = res->body.asset().amount;
        } else {
            asset_supply = 0;
        }
    } else {
        asset_supply = 0;
    }
    trie::SnapshotTrieMetadataBase::from_value(obj);
}

SisyphusStateDB::SisyphusStateMetadata&
SisyphusStateDB::SisyphusStateMetadata::operator+=(
    const SisyphusStateMetadata& other)
{
    asset_supply += other.asset_supply;
    trie::SnapshotTrieMetadataBase::operator+=(other);
    return *this;
} */

std::optional<StorageObject>
SisyphusStateDB::get_committed_value(const AddressAndKey& a)
{
    auto const* res = state_db.get_value(a);
    if (res) {
        return (res)->get_committed_object();
    }
    return std::nullopt;
}

void
do_nothing_if_merge(const SisyphusStateDB::value_t& value)
{}

std::optional<RevertableObject::DeltaRewind>
SisyphusStateDB::try_apply_delta(const AddressAndKey& a,
                                 const StorageDelta& delta)
{
    auto* res = state_db.get_value(a, true);

    if (!res) {
        auto* root = state_db.get_root_and_invalidate_hash(current_timestamp);

        root->template insert<&do_nothing_if_merge>(a, state_db.get_gc(), current_timestamp, state_db.get_storage());

        res = state_db.get_value(a, true);
    }

    if (res == nullptr) {
        throw std::runtime_error("should be impossible");
    }

    return (res)->try_add_delta(delta);
}


struct SisyphusUpdateFn
{
    SisyphusStateDB::trie_t& main_db;
    uint32_t current_timestamp;

    using prefix_t = SisyphusStateDB::prefix_t;
    using index_trie_t = TypedModificationIndex::map_t;

    void operator()(index_trie_t::const_applyable_ref& work_root)
    {
        // WARNING -- don't save these pointers -- they might get deleted during
        // the normalization cleanup, and regardless accessing them later
        // doesn't guarantee hash invalidation.

        static_assert(!std::is_same<decltype(work_root.get_prefix()), prefix_t>::value, "not same");
        auto* main_db_subnode = main_db.get_subnode_ref_and_invalidate_hash(
            work_root.get_prefix(), work_root.get_prefix_len(), current_timestamp);

        if (main_db_subnode->get_prefix_len() != std::min(prefix_t::len(), work_root.get_prefix_len()))
        {
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
                                const prefix_t& addrkey) {
            auto* main_db_value = main_db_subnode->get_value(addrkey, main_db.get_storage(), true);
            if (main_db_value) {
                main_db_subnode->invalidate_hash_to_key(addrkey, current_timestamp);

                main_db_value->commit_round();

                // TODO this check shouldn't be necessary
                if (!(main_db_value->get_committed_object())) {
                    main_db_subnode->delete_value(addrkey, current_timestamp, main_db.get_gc(), main_db.get_storage());
                }
            } else {
                std::printf("there was no value returned from %s\n", addrkey.to_string(prefix_t::len()).c_str());
                std::fflush(stdout);
                throw std::runtime_error("should never happen");
            }
        };

        // preallocating reuseable buffer helps performance
        std::vector<uint8_t> digest_bytes;

        work_root.apply_to_keys(apply_lambda, prefix_t::len());
        main_db_subnode->compute_hash_and_normalize(main_db.get_gc(), 0,
                                                    digest_bytes, main_db.get_storage());
    }
};

/**
 * Helper class for rewinding a transaction block that gets rejected.
 * Relies on the fact that values aren't inserted into StateDB before
 * commitment of a block.
 */
struct SisyphusRewindFn
{
    SisyphusStateDB::trie_t& main_db;
    uint32_t current_timestamp;

    using prefix_t = SisyphusStateDB::prefix_t;

    template<typename Applyable>
    void operator()(const Applyable& work_root)
    {
        auto* main_db_subnode = main_db.get_subnode_ref_and_invalidate_hash(
            work_root.get_prefix(), work_root.get_prefix_len(), current_timestamp);

        auto apply_lambda = [this, main_db_subnode](const prefix_t& addrkey) {
            auto* main_db_value
                = main_db_subnode->get_value(addrkey, main_db.get_storage(), true);
            if (main_db_value) {
                // no need to invalidate hashes when rewinding,
                // as we are merely rewinding to committed values.
                main_db_value->rewind_round();
            } else {
                // don't need to do anything, but this should never happen
                throw std::runtime_error("invalid nonsense");
            }
        };

        work_root.apply_to_keys(apply_lambda, prefix_t::len());
    }
};

void
SisyphusStateDB::commit_modifications(const TypedModificationIndex& list)
{
    auto ts = utils::init_time_measurement();

    SisyphusUpdateFn update(state_db, current_timestamp);

    std::printf("parallel modify before %lf\n", utils::measure_time(ts));

    uint32_t grain_size = std::max<uint32_t>(1, list.size() / 1000);
    list.get_keys()
        .parallel_batch_value_modify_const<SisyphusUpdateFn, prefix_t::len()>(
            update, grain_size);

    std::printf("parallel modify after %lf\n", utils::measure_time(ts));

    state_db.hash_and_normalize(0);

    std::printf("root hash %lf\n", utils::measure_time(ts));

    state_db.do_gc();
    std::printf("finish %lf\n", utils::measure_time(ts));
}

void
SisyphusStateDB::rewind_modifications(const TypedModificationIndex& list)
{
    SisyphusRewindFn rewind(state_db, current_timestamp);

    list.get_keys()
        .parallel_batch_value_modify_const<SisyphusRewindFn, prefix_t::len()>(rewind,
                                                                      1);
    state_db.hash_and_normalize(0);
    state_db.do_gc();
}

Hash
SisyphusStateDB::hash()
{
    auto h = state_db.hash_and_normalize(0);
    Hash out;
    std::memcpy(out.data(), h.data(), h.size());
    return out;
}

} // namespace scs
