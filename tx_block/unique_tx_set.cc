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
    static void
    value_insert(UniqueTxSet::value_t& main_entry,
                             TxSetEntry&& other_entry)
    {
        if (main_entry.nondeterministic_results.size() == 0)
        {
            main_entry = other_entry;
        }

        if (main_entry.nondeterministic_results.size() == 0)
        {
            throw std::runtime_error("invalid insert in unique tx set");
        }
    }

    static TxSetEntry new_value(UniqueTxSet::prefix_t const& key)
    {
        return TxSetEntry{};
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

static TxSetEntry const& get_txset_entry(trie::ByteArrayPrefix<sizeof(Hash)> const&, const TxSetEntry& entry)
{
    return entry;
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
