#pragma once

#include "mtt/trie/recycling_impl/trie.h"
#include "mtt/trie/recycling_impl/atomic_trie.h"

#include <utils/threadlocal_cache.h>

#include "xdr/types.h"

#include "config/static_constants.h"

namespace scs {

class ModifiedKeysList
{
    using value_t = trie::EmptyValue;
    using trie_prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey)>;

   // using metadata_t = void;

    using map_t = trie::AtomicTrie<value_t, trie_prefix_t>;

    //using map_t = trie::RecyclingTrie<value_t, trie_prefix_t, metadata_t>;

    map_t keys;
    using serial_trie_t = trie::AtomicTrieReference<value_t, trie_prefix_t>;

    //using serial_trie_t = map_t::serial_trie_t;

    using cache_t = utils::ThreadlocalCache<serial_trie_t, TLCACHE_SIZE>;

    cache_t cache;

    bool logs_merged = false;

    void assert_logs_merged() const;
    void assert_logs_not_merged() const;

  public:
    void log_key(const AddressAndKey& key);

    void merge_logs();

    const map_t& get_keys() const
    {
        assert_logs_merged();
        return keys;
    }

    size_t size() const {
        return keys.size();
    }

    void clear();

    Hash hash();
};

} // namespace scs
