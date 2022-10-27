#include "tx_block/tx_set.h"

using xdr::operator==;

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

struct MultiplicityAddInsertFn : public trie::GenericInsertFn<TxSet::value_t>
{
    static void 
    value_insert(TxSet::value_t& main_value, TxSet::value_t&& other_value) {
        if (main_value.multiplicity == 0)
        {
            main_value.tx = other_value.tx;
        }
        if (other_value.multiplicity > 0 && main_value.tx != other_value.tx)
        {
            throw std::runtime_error("tx mismatch");
        }
        main_value.multiplicity += other_value.multiplicity;
    }
};

struct MultiplicityAddMergeFn 
{
    using value_t = TxSet::value_t;

    template<typename MetadataType>
    static
    MetadataType value_merge_recyclingimpl(value_t& main_value, const value_t& other_value) {
        if (main_value.multiplicity == 0)
        {
            main_value.tx = other_value.tx;
        }
        if (other_value.multiplicity > 0 && main_value.tx != other_value.tx)
        {
            throw std::runtime_error("tx mismatch");
        }
        main_value.multiplicity += other_value.multiplicity;
        return MetadataType::zero();
    }
};

void
TxSet::add_transaction(const Hash& hash, const Transaction& tx)
{
    assert_txs_not_merged();
    auto& local_trie = cache.get(txs);
    local_trie.template insert<MultiplicityAddInsertFn, TxSetEntry>(hash, TxSetEntry(tx, 1));
}

void
TxSet::finalize()
{
    assert_txs_not_merged();
    txs.template batch_merge_in<MultiplicityAddMergeFn>(cache);
    txs_merged = true;
}

} // namespace scs