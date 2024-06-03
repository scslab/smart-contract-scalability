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

#include <xdrpp/marshal.h>

#include "xdr/types.h"
#include "xdr/storage_delta.h"

#include "config/static_constants.h"

#include "state_db/modification_metadata.h"

namespace scs
{
class UniqueTxSet;

class TypedModificationIndex
{

	static void serialize(std::vector<uint8_t>& buf, const PrioritizedStorageDelta& v)
    {
        xdr::append_xdr_to_opaque(buf, v);
    }

public:
    using value_t = trie::BetterSerializeWrapper<PrioritizedStorageDelta, &serialize>;
    // keys are [addrkey] [modification type] [modification] [txid]
    constexpr static size_t modification_key_length = 32 + 8 + 8; // len(hash) + len(tag) + len(within-typeclass priority), for hashset entries

    using trie_prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey) + 1 + modification_key_length + sizeof(Hash)>;
    using map_t = trie::AtomicTrie<value_t, trie_prefix_t, ModificationMetadata>;

private:
    map_t keys;
    using serial_trie_t = trie::AtomicTrieReference<map_t>;

    using cache_t = utils::ThreadlocalCache<serial_trie_t, TLCACHE_SIZE>;

    cache_t cache;

    bool hashed = false;
    void assert_hashed() const
    {
        if (!hashed) {
            throw std::runtime_error("not hashed");
        }
    }

public:

	void log_modification(AddressAndKey const& addrkey, PrioritizedStorageDelta const& mod, Hash const& src_tx_hash);

    const map_t& get_keys() const
    {
        return keys;
    }

    size_t size() const {
    	return keys.size();
    }

    void save_modifications(ModIndexLog& out);

    Hash hash();
	void clear();

    void prune_conflicts(UniqueTxSet& txs) const;
};


// public for testing
TypedModificationIndex::trie_prefix_t
make_index_key(AddressAndKey const& addrkey, PrioritizedStorageDelta const& mod, Hash const& src_tx_hash);

}