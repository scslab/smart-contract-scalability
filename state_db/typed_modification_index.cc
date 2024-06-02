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


#include "tx_block/unique_tx_set.h"
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
               uint64_t priority,
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
    std::memcpy(data, &priority, sizeof(uint64_t));
    data += sizeof(uint64_t);
    std::memcpy(data, src_tx_hash.data(), sizeof(Hash));
    return out;
}

void 
TypedModificationIndex::log_modification(
    AddressAndKey const& addrkey, StorageDelta const& mod, uint64_t priority, Hash const& src_tx_hash)
{
    auto& local_trie = cache.get(keys);
    auto key = make_index_key(addrkey, value_t(mod), priority, src_tx_hash);

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
    hashed = true;
    return out;
}

void 
TypedModificationIndex::clear()
{
    hashed = false;
    cache.clear();
    keys.clear();
}

struct CancelTxsFn
{
    
    UniqueTxSet& txs;

    using index_trie_t = TypedModificationIndex::map_t;
    using prefix_t = TypedModificationIndex::trie_prefix_t;

    index_trie_t const& modification_trie_ref;

    void operator()(index_trie_t::const_applyable_ref& work_root)
    {
        // WARNING -- don't save these pointers -- they might get deleted during
        // the normalization cleanup, and regardless accessing them later
        // doesn't guarantee hash invalidation.

        static_assert(std::is_same<decltype(work_root.get_prefix()), prefix_t>::value, "is same");

        prefix_t work_root_prefix = work_root.get_prefix();




        /*

        auto* main_db_subnode = main_db.get_subnode_ref_and_invalidate_hash(
            work_root_prefix, statedb_query_len, current_timestamp);

        if (main_db_subnode->get_prefix_len() != statedb_query_len)
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
                                                    */
    }
};

void
TypedModificationIndex::prune_conflicts(UniqueTxSet& txs) const
{
    assert_hashed();
    CancelTxsFn cancels(txs);

    keys.parallel_batch_value_modify_const<CancelTxsFn, trie_prefix_t::len()>(cancels, 1);




}

} // namespace scs
