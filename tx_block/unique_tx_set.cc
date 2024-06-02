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

#include "tx_block/unique_tx_set.h"

#include <xdrpp/types.h>

#include "crypto/hash.h"

namespace scs {

using xdr::operator==;
using xdr::operator!=;

struct UniqueInsertFn
{
    // Value insert normally needs to be threadsafe.
    // However, the use case here is implicitly guarded by 
    // how tries create new value nodes.
    // Either they are created by (reset_value) then (value_insert) (guarded by the fact
    // that during this process they're not actually in the trie yet)
    // or, once they're in the trie, this function is always read-only (the first condition always
    // gets skipped, since it only does something on the first call to value_insert)
    static void
    value_insert(UniqueTxSet::value_t& main_entry,
                             TxSetEntry&& other_entry)
    {
        // main entry is empty
        if (main_entry.entry.nondeterministic_results.size() == 0)
        {
            main_entry.entry = other_entry;
        }

        // nonsense
        if (main_entry.entry.nondeterministic_results.size() == 0)
        {
            throw std::runtime_error("invalid insert in unique tx set");
        }
        // otherwise, null op, and function returns false -- i.e. tx was not inserted (since it's already there)
    }

    static void reset_value(UniqueTxSet::value_t& v, UniqueTxSet::prefix_t const& key)
    {
        v.cancelled.store(false, std::memory_order_relaxed);
        v.entry = TxSetEntry();
    }
};

void
UniqueTxSet::assert_txs_merged() const
{
    if (!txs_merged) {
        throw std::runtime_error("txs not merged");
    }
}
void
UniqueTxSet::assert_txs_not_merged() const
{
    if (txs_merged) {
        throw std::runtime_error("txs already merged");
    }
}

void
UniqueTxSet::clear()
{
    cache.clear();
    txs.clear();
    txs_merged = false;
}

static TxSetEntry const& get_txset_entry(trie::ByteArrayPrefix<sizeof(Hash)> const&, const CancellableTxSetEntry& entry)
{
    return entry.entry;
}

bool
UniqueTxSet::try_add_transaction(const Hash& hash, const SignedTransaction& tx, const NondeterministicResults& nres)
{
    assert_txs_not_merged();
    auto& local_trie = cache.get(txs);
    return local_trie.template insert<UniqueInsertFn, TxSetEntry>(
        hash, TxSetEntry(tx, xdr::xvector<NondeterministicResults>{nres}));
}

void
UniqueTxSet::cancel_transaction(const Hash& h)
{
    auto* r = txs.get_value(h);
    if (r == nullptr) {
        throw std::runtime_error("cannot cancel nexist tx");
    }

    r -> cancelled.store(true, std::memory_order_relaxed);
}

void
UniqueTxSet::finalize()
{
    assert_txs_not_merged();
    txs_merged = true;
}

void 
UniqueTxSet::serialize_block(Block& out) const
{
    assert_txs_merged();

    txs.accumulate_values_parallel<decltype(out.transactions), get_txset_entry>(out.transactions, 10);
}

Hash
UniqueTxSet::hash()
{
    assert_txs_merged();
    Hash out;

    auto h = txs.hash();
    std::memcpy(out.data(),
        h.data(),
        h.size());

    return out;
}

} // namespace scs
