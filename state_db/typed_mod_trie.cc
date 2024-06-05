#include "state_db/typed_mod_trie.h"


#include "tx_block/unique_tx_set.h"

#include "state_db/deterministic_state_db.h"

using xdr::operator==;

namespace scs 
{

void TypedModificationTrie::prune_all_below(node_t const* node,
    UniqueTxSet& txs) const
{
    auto const& prefix_len = node->get_prefix_len();

    if (prefix_t::len() != prefix_len) {
        for (uint8_t bb = 0; bb < 16; bb++) {
            const node_t* child = node->get_child(bb, allocator);
            if (child != nullptr) {
                prune_all_below(
                    child, txs);
            }
        }
        return;
    }

    Hash tx;
    auto const* ptr = node->get_prefix().underlying_data_ptr();

    std::memcpy(tx.data(), ptr - tx.size(), tx.size());

    txs.cancel_transaction(tx);
}

/**
 * It's not safe to just call this over the entire trie.
 * It would be (given appropriate metadata tracking in the tbb range)
 * except that the metadata does not track hashsset insert uniqueness.
 * That has to be checked with the extra flag in the input.
 */

void
TypedModificationTrie::prune_conflicts(node_t const* node,
                                       std::optional<ModificationMetadata> offset_meta,
                                       const DeterministicStateDBValue* value,
                                       DeterministicStateDB const& sdb,
                                       UniqueTxSet& txs,
                                       bool hs_duplicate_insert_flag) const
{
    constexpr size_t MIN_CHECK_LEN_BITS = sizeof(AddressAndKey) * 8;
    constexpr size_t TC_SELECTOR_BYTE = sizeof(AddressAndKey);

    auto const& prefix = node->get_prefix();
    auto const& prefix_len = node->get_prefix_len();

    const uint8_t* raw_data = prefix.underlying_data_ptr();

    // meaningless (zeroed out) if prefix_len.len <= MIN_CHECK_LEN_BITS
    DeltaType selector = DeltaType(raw_data[TC_SELECTOR_BYTE]);

    auto get_full_typeclass = [&] () -> std::optional<CompressedStorageDeltaClass>
    {

        if (prefix_len.len <= MIN_CHECK_LEN_BITS + 8) {
            return std::nullopt;
        }



        std::optional<CompressedStorageDeltaClass> out = CompressedStorageDeltaClass();

        switch(selector)
        {
        case DeltaType::HASH_SET_INSERT:
        case DeltaType::HASH_SET_INCREASE_LIMIT:
        case DeltaType::HASH_SET_CLEAR:
        case DeltaType::HASH_SET_INSERT_RESERVE_SIZE:
            out->type(ObjectType::HASH_SET);
            return out;
        case DeltaType::ASSET_OBJECT_ADD:
            out->type(ObjectType::KNOWN_SUPPLY_ASSET);
            return out;
        case DeltaType::NONNEGATIVE_INT64_SET_ADD:
        {
            out->type(ObjectType::NONNEGATIVE_INT64);
            if (prefix_len.len < MIN_CHECK_LEN_BITS + 8 + 64)
            {
                return std::nullopt;
            }
            uint64_t val;
            utils::read_unsigned_big_endian(
                raw_data + sizeof(AddressAndKey) + 1, val);
            out -> nonnegative_int64() = static_cast<int64_t>(val);
            return out;
        }
        case DeltaType::RAW_MEMORY_WRITE:
        {
            out->type(ObjectType::RAW_MEMORY);
            if (prefix_len.len < MIN_CHECK_LEN_BITS + 8 + 256)
            {
                return std::nullopt;
            }
            std::memcpy(out->hashed_data().data(), raw_data + sizeof(AddressAndKey) + 1, sizeof(Hash));
            return out;
        }
        case DeltaType::DELETE_LAST:
            throw std::runtime_error("should not get_tc on a DELETE_LAST");
        }

        throw std::runtime_error("unknown typeclass");
    };

    if (prefix_len.len <= MIN_CHECK_LEN_BITS) {
        for (uint8_t bb = 0; bb < 16; bb++) {
            const node_t* child = node->get_child(bb, allocator);
            if (child != nullptr) {
                prune_conflicts(
                    child, std::nullopt, nullptr, sdb, txs, false);
            }
        }
        return;
    }

    // prevent typeclass matching on DELETE_LAST
    if (selector == DeltaType::DELETE_LAST)
    {
        return;
    }

    // must be bits in prefix to recover the AddressAndKey
    if (value == nullptr) {
        AddressAndKey query_key;
        node->get_prefix().write_bytes_to(query_key);

        value = sdb.get_trie().get_value(query_key);

        if (value == nullptr) {
            throw std::runtime_error("invalid lookup");
        }
    }

    // not enough bits in parent node to have recovered the typeclass
    if (!offset_meta.has_value())
    {
        auto tc = get_full_typeclass();
        if (!tc)
        {
            if (prefix_len == prefix_t::len())
            {
                throw std::runtime_error("full prefix should have tc");
            }

            for (uint8_t bb = 0; bb < 16; bb++) {
                const node_t* child = node->get_child(bb, allocator);
                if (child != nullptr) {
                    prune_conflicts(
                        child, std::nullopt, value, sdb, txs, false);
                }
            }
            return;
        }

        // Invariant -- if we've gotten the full typeclass from the key at a parent node,
        // then we need not check the tc below

        if (value -> tc.get_sdc() != tc.value())
        {
            // prune all children -- mismatched typeclass
            prune_all_below(node, txs);
            return;
        }
        offset_meta.emplace();
    }

    // offset_meta is nonnull, meaning we have & match full typeclass
    
    constexpr  size_t HS_INSERT_FULL_LEN_BYTES = sizeof(AddressAndKey) + 1 + sizeof(Hash);

    switch(selector)
    {
    case DeltaType::DELETE_LAST:
        throw std::runtime_error("should not happen here");
    case DeltaType::RAW_MEMORY_WRITE:
        return;
    case DeltaType::NONNEGATIVE_INT64_SET_ADD:
    {
        if (prefix_len != prefix_t::len())
        {
            for (uint8_t bb = 0; bb < 16; bb++)
            {
                const node_t* child = node -> get_child(bb, allocator);
                if (child != nullptr) {
                    prune_conflicts(child, offset_meta, value, sdb, txs, false);
                    (*offset_meta) += child->get_metadata();
                }
            }
            return;
        }

        __int128 prev_sum = offset_meta -> get_nnint64();
        __int128 set_value = value -> tc.get_sdc().nonnegative_int64();

        auto const& mod_value = node -> get_value_self(allocator);

        int64_t delta = mod_value.set_add_nonnegative_int64().delta;
        if (delta > 0) {
            return;
        }

        prev_sum += delta;

        if (prev_sum + set_value < 0) {
            prune_all_below(node, txs);
        }

        return;
    }
    case DeltaType::HASH_SET_INSERT:
    {
        if (hs_duplicate_insert_flag) {
            prune_all_below(node, txs);
            return;
        }

        bool duplicate_insert = false;

        if (prefix_len != prefix_t::len() && prefix_len.len >= (8 * HS_INSERT_FULL_LEN_BYTES))
        {
            duplicate_insert = true;
        } 

        bool first = true;
        for (uint8_t bb = 0; bb < 16; bb++)
        {
            const node_t* child = node -> get_child(bb, allocator);
            if (child != nullptr) {
                if (first)
                {
                    prune_conflicts(child, offset_meta, value, sdb, txs, false);
                } else
                {
                    if (duplicate_insert)
                    {
                        prune_all_below(child, txs);
                        first = false;
                    }
                }

                (*offset_meta) += child->get_metadata();
            }
        }
        return;
    }
    case DeltaType::HASH_SET_INCREASE_LIMIT:
    case DeltaType::HASH_SET_CLEAR:
    {
        return;
    }
    case DeltaType::HASH_SET_INSERT_RESERVE_SIZE:
    {
        if (prefix_len != prefix_t::len())
        {
            for (uint8_t bb = 0; bb < 16; bb++)
            {
                const node_t* child = node -> get_child(bb, allocator);
                if (child != nullptr) {
                    prune_conflicts(child, offset_meta, value, sdb, txs, false);
                    (*offset_meta) += child->get_metadata();
                }
            }
            return;
        }

        unsigned __int128 prev_sum = offset_meta -> get_hs_insert_count();
        auto const& obj = value -> object.get_committed_object();
        unsigned __int128 limit = 0;
        if (obj.has_value())
        {
            limit = obj->body.hash_set().max_size;
        }

        auto const& mod_value = node -> get_value_self(allocator);

        uint64_t delta = mod_value.reserve_amount();

        prev_sum += delta;

        if (prev_sum > limit) {
            prune_all_below(node, txs);
        }

        return;
    }
    default:
        throw std::runtime_error("unimpl deltatype");

    }

    throw std::runtime_error("Should not reach end of switch");
}

} // namespace scs