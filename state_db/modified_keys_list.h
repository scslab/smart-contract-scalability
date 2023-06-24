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

#include "mtt/ephemeral_trie/atomic_ephemeral_trie.h"

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
    using serial_trie_t = trie::AtomicTrieReference<map_t>;//value_t, trie_prefix_t>;

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
