#include "tx_block/tx_set.h"

namespace scs {
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
TxSet::add_transaction(const Hash& hash, const Transaction& tx)
{
    assert_txs_not_merged();
    auto& local_trie = cache.get(txs);
    local_trie.insert(hash, value_t(tx));
}

void
TxSet::finalize()
{
    assert_txs_not_merged();
    txs.template batch_merge_in<trie::NoDuplicateKeysMergeFn>(cache);
    txs_merged = true;
}

} // namespace scs