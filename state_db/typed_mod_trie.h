#pragma once

#include "mtt/ephemeral_trie/atomic_ephemeral_trie.h"

#include <utils/threadlocal_cache.h>

#include <xdrpp/marshal.h>

#include <vector>
#include <cstdint>

#include "xdr/storage_delta.h"

#include "state_db/modification_metadata.h"

#include "config/static_constants.h"

namespace scs
{

struct TypedModificationTrieTypes {

	static void serialize(std::vector<uint8_t>& buf, const StorageDelta& v)
    {
        xdr::append_xdr_to_opaque(buf, v);
    }

    using value_t = trie::BetterSerializeWrapper<StorageDelta, &serialize>;

    // keys are [addrkey] [modification type] [modification] [priority] [txid]
    constexpr static size_t modification_key_length = 32 + 8; // len(hash) + len(tag), for hashset entries

    using trie_prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey) + 1 + modification_key_length + sizeof(uint64_t) + sizeof(Hash)>;
    using map_t = trie::AtomicTrie<value_t, trie_prefix_t, ModificationMetadata>;

    using serial_trie_t = trie::AtomicTrieReference<map_t>;

    using cache_t = utils::ThreadlocalCache<serial_trie_t, TLCACHE_SIZE>;
};


class TypedModificationTrie : public TypedModificationTrieTypes::map_t
{

	public:

	void prune_conflicts(node_t const* node, ModificationMetadata offset_meta) const;
};





}