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

#include "mtt/trie/utils.h"

namespace scs {

struct CancellableTxSetEntry {
    std::atomic<bool> cancelled = false;
    TxSetEntry entry;

    CancellableTxSetEntry(TxSetEntry e)
        : entry(e)
        {}

    CancellableTxSetEntry() {};
};

class UniqueTxSet
{
    static std::vector<uint8_t> serialize(const CancellableTxSetEntry& v)
    {
        return xdr::xdr_to_opaque(v.entry);
    }

    friend struct UniqueInsertFn;

    using value_t = trie::XdrTypeWrapper<CancellableTxSetEntry, &serialize>;

    using prefix_t = trie::ByteArrayPrefix<sizeof(Hash)>;

    using map_t = trie::AtomicTrie<value_t, prefix_t>;

    map_t txs;

    using cache_t = utils::ThreadlocalCache<trie::AtomicTrieReference<map_t>, TLCACHE_SIZE>;

    cache_t cache;

    bool txs_merged = false;

    void assert_txs_merged() const;
    void assert_txs_not_merged() const;

  public:

    bool try_add_transaction(const Hash& hash, const SignedTransaction& tx, const NondeterministicResults& nres);

    void cancel_transaction(const Hash& hash);

    void finalize();

    void serialize_block(Block& out) const;

    Hash hash();

    void clear();

    // for testing

    bool contains_tx(const Hash& hash) const
    {
        auto const* r = txs.get_value(hash);

        return (r != nullptr);
    }
};

} // namespace scs
