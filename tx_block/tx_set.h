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

#include "mtt/common/prefix.h"
#include "mtt/ephemeral_trie/atomic_ephemeral_trie.h"

#include <utils/threadlocal_cache.h>

#include "xdr/transaction.h"
#include "xdr/types.h"
#include "xdr/block.h"

#include <xdrpp/marshal.h>

#include <mutex>

#include "config/static_constants.h"

namespace scs {

class LockableTxSetEntry
{
    std::mutex mtx;
    TxSetEntry entry;

public:

    LockableTxSetEntry()
        : mtx()
        , entry()
        {}

    LockableTxSetEntry& operator=(TxSetEntry&& other)
    {
        std::lock_guard lock(mtx);
        entry = std::move(other);
        return *this;
    }

    std::lock_guard<std::mutex>
    lock() {
        return {mtx, std::adopt_lock};
    }

    TxSetEntry& get()
    {
        return entry;
    }

    const TxSetEntry& get() const
    {
        return entry;
    }

    void copy_data(std::vector<uint8_t>& buf) const {
        auto serialization = xdr::xdr_to_opaque(entry);
        buf.insert(buf.end(), serialization.begin(), serialization.end());
    }
};

class TxSet
{
 //   static std::vector<uint8_t> serialize(const TxSetEntry& v)
 //   {
 //       return xdr::xdr_to_opaque(v);
 //   }

    friend struct ResultListInsertFn;

    using value_t = LockableTxSetEntry;  //trie::XdrTypeWrapper<TxSetEntry, &serialize>;

    using prefix_t = trie::ByteArrayPrefix<sizeof(Hash)>;

    //using metadata_t = void;

    //using map_t = trie::RecyclingTrie<value_t, trie_prefix_t, metadata_t>;

    using map_t = trie::AtomicTrie<value_t, prefix_t>;

    map_t txs;

    //using serial_trie_t = map_t::serial_trie_t;

    using cache_t = utils::ThreadlocalCache<trie::AtomicTrieReference<map_t>, TLCACHE_SIZE>;

    cache_t cache;

    bool txs_merged = false;

    void assert_txs_merged() const;
    void assert_txs_not_merged() const;

  public:
    bool try_add_transaction(const Hash& hash, const SignedTransaction& tx, const NondeterministicResults& nres);

    void finalize();

    void serialize_block(Block& out) const;

    Hash hash();

    void clear();

    // for testing

    uint32_t contains_tx(const Hash& hash) const
    {
        auto const* r = txs.get_value(hash);
        if (r == nullptr)
        {
            return 0;
        }
        return r -> get().nondeterministic_results.size();
    }
};

} // namespace scs
