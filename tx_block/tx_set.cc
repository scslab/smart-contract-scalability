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

#include "tx_block/tx_set.h"

#include <xdrpp/types.h>

namespace scs {

using xdr::operator==;
using xdr::operator!=;

void
TxSet::assert_txs_merged() const
{
    if (!txs_merged) {
        throw std::runtime_error("txs not merged");
    }
}
void
TxSet::assert_txs_not_merged() const
{
    if (txs_merged) {
        throw std::runtime_error("txs already merged");
    }
}

void
TxSet::clear()
{
    cache.clear();
    txs.clear();
    txs_merged = false;
}

struct ResultListInsertFn
{
    static void value_insert(TxSet::value_t& main_value,
                             TxSetEntry&& other_entry)
    {

        auto lock = main_value.lock();

        auto& main_entry = main_value.get();


        if (main_entry.nondeterministic_results.size() == 0) {
            main_entry.tx = other_entry.tx;
        }
        if (other_entry.nondeterministic_results.size() > 0 && main_entry.tx != other_entry.tx) {
            throw std::runtime_error("tx mismatch");
        }
        main_entry.nondeterministic_results.insert(
            main_entry.nondeterministic_results.end(), 
            other_entry.nondeterministic_results.begin(), 
            other_entry.nondeterministic_results.end());
    }

    static TxSetEntry new_value(TxSet::prefix_t const& key)
    {
        return TxSetEntry{};
    }
};

static TxSetEntry const& get_txset_entry(trie::ByteArrayPrefix<sizeof(Hash)> const&, const LockableTxSetEntry& entry)
{
    return entry.get();
}

bool
TxSet::try_add_transaction(const Hash& hash, const SignedTransaction& tx, const NondeterministicResults& nres)
{
    assert_txs_not_merged();
    auto& local_trie = cache.get(txs);
    local_trie.template insert<ResultListInsertFn, TxSetEntry>(
        hash, TxSetEntry(tx, xdr::xvector<NondeterministicResults>{nres}));
    return true;
}

void
TxSet::finalize()
{
    assert_txs_not_merged();
    txs_merged = true;
}

void 
TxSet::serialize_block(Block& out) const
{
    assert_txs_merged();

    txs.accumulate_values_parallel<decltype(out.transactions), get_txset_entry>(out.transactions, 10);
}

Hash
TxSet::hash()
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
