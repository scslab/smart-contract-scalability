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

#include "state_db/state_db_v2.h"

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

#include "state_db/modified_keys_list.h"

#include <utils/assert.h>
#include <utils/serialize_endian.h>
#include <utils/time.h>

namespace scs {

std::optional<StorageObject>
StateDBv2::get_committed_value(const AddressAndKey& a) const
{
    auto const* res = state_db.get_value(a);
    if (res) {
        return (res)->get_committed_object();
    }
    return std::nullopt;
}

void
do_nothing_if_merge(
    const StateDBv2::value_t& value) {}

std::optional<RevertableObject::DeltaRewind>
StateDBv2::try_apply_delta(const AddressAndKey& a, const StorageDelta& delta)
{
    auto* res = state_db.get_value(a);

    if (!res) {
        auto* root = state_db.get_root_and_invalidate_hash();

        root->template insert<&do_nothing_if_merge>(
                a, state_db.get_gc());

        res = state_db.get_value(a);
    }

    if (res == nullptr) {
        throw std::runtime_error("should be impossible");
    }

    return (res)->try_add_delta(delta);
}

struct UpdateFnv2
{
    StateDBv2::trie_t& main_db;
    using prefix_t = StateDBv2::prefix_t;

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

                // TODO this check shouldn't be necessary
                if (!(main_db_value->get_committed_object())) {
                    main_db_subnode->delete_value(addrkey, main_db.get_gc());
                }
            } else {
                std::printf("there was no value returned from %s\n", addrkey.to_string(prefix_t::len()).c_str());
                std::fflush(stdout);
                throw std::runtime_error("should never happen");
             /*   AddressAndKey query
                    = addrkey.template get_bytes_array<AddressAndKey>();

                std::optional<StorageObject> new_obj
                    = new_key_cache.commit_and_get(query);

                if (new_obj) {
                    main_db_subnode->template insert<&merge_impossible>(
                        addrkey, main_db.get_gc(), *new_obj);
                } */
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
StateDBv2::commit_modifications(const ModifiedKeysList& list)
{
    auto ts = utils::init_time_measurement();

    UpdateFnv2 update(state_db);

    std::printf("parallel modify before %lf\n", utils::measure_time(ts));

    uint32_t grain_size = std::max<uint32_t>(1, list.size() / 1000);
    list.get_keys()
        .parallel_batch_value_modify_const<UpdateFnv2, prefix_t::len()>(
            update, grain_size);

    std::printf("parallel modify after %lf\n", utils::measure_time(ts));

    state_db.hash_and_normalize();

    std::printf("clear and root hash %lf\n", utils::measure_time(ts));

    state_db.do_gc();
    std::printf("finish %lf\n", utils::measure_time(ts));
}


/**
 * Helper class for rewinding a transaction block that gets rejected.
 * Relies on the fact that values aren't inserted into StateDB before
 * commitment of a block.
 */
struct RewindFnv2
{
    StateDBv2::trie_t& main_db;

    using prefix_t = StateDBv2::prefix_t;

    template<typename Applyable>
    void operator()(const Applyable& work_root)
    {
        auto* main_db_subnode = main_db.get_subnode_ref_and_invalidate_hash(
            work_root.get_prefix(), work_root.get_prefix_len());

        auto apply_lambda = [this, main_db_subnode](const prefix_t& addrkey,
                                                    const trie::EmptyValue&) {
            auto* main_db_value
                = main_db_subnode->get_value(addrkey);
            if (main_db_value) {
                // no need to invalidate hashes when rewinding,
                // as we are merely rewinding to committed values.
                main_db_value->rewind_round();
            } else {
                // don't need to do anything, but this should never happen
                throw std::runtime_error("invalid nonsense");
            }
        };

        work_root.apply_to_kvs(apply_lambda);
    }
};

void
StateDBv2::rewind_modifications(const ModifiedKeysList& list)
{
    RewindFnv2 rewind(state_db);

    list.get_keys()
        .parallel_batch_value_modify_const<RewindFnv2, prefix_t::len()>(rewind,
                                                                      1);
    state_db.hash_and_normalize();
    state_db.do_gc();
}

Hash
StateDBv2::hash()
{
    auto h = state_db.hash_and_normalize();
    Hash out;
    std::memcpy(out.data(), h.data(), h.size());
    return out;
}

} // namespace scs
