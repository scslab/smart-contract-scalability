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

/*

struct MultiplicityAddInsertFn : public trie::GenericInsertFn<TxSet::value_t>
{
    static void value_insert(TxSet::value_t& main_value,
                             TxSet::value_t&& other_value)
    {
        if (main_value.multiplicity == 0) {
            main_value.tx = other_value.tx;
        }
        if (other_value.multiplicity > 0 && main_value.tx != other_value.tx) {
            throw std::runtime_error("tx mismatch");
        }
        main_value.multiplicity += other_value.multiplicity;
    }
};

struct MultiplicityAddMergeFn
{
    using value_t = TxSet::value_t;

    template<typename MetadataType>
    static MetadataType value_merge_recyclingimpl(value_t& main_value,
                                                  const value_t& other_value)
    {
        if (main_value.multiplicity == 0) {
            main_value.tx = other_value.tx;
        }
        if (other_value.multiplicity > 0 && main_value.tx != other_value.tx) {
            throw std::runtime_error("tx mismatch");
        }
        main_value.multiplicity += other_value.multiplicity;
        return MetadataType::zero();
    }
}; */

struct ResultListInsertFn : public trie::GenericInsertFn<TxSet::value_t>
{
    static void value_insert(TxSet::value_t& main_value,
                             TxSet::value_t&& other_value)
    {
        if (main_value.nondeterministic_results.size() == 0) {
            main_value.tx = other_value.tx;
        }
        if (other_value.nondeterministic_results.size() > 0 && main_value.tx != other_value.tx) {
            throw std::runtime_error("tx mismatch");
        }
        main_value.nondeterministic_results.insert(
            main_value.nondeterministic_results.end(), 
            other_value.nondeterministic_results.begin(), 
            other_value.nondeterministic_results.end());
    }
};

struct ResultListMergeFn
{
    using value_t = TxSet::value_t;

    template<typename MetadataType>
    static MetadataType value_merge_recyclingimpl(value_t& main_value,
                                                  const value_t& other_value)
    {
        if (main_value.nondeterministic_results.size() == 0) {
            main_value.tx = other_value.tx;
        }
        if (other_value.nondeterministic_results.size() > 0 && main_value.tx != other_value.tx) {
            throw std::runtime_error("tx mismatch");
        }
        main_value.nondeterministic_results.insert(
            main_value.nondeterministic_results.end(), 
            other_value.nondeterministic_results.begin(), 
            other_value.nondeterministic_results.end());
        return MetadataType::zero();
    }
};

void
TxSet::add_transaction(const Hash& hash, const SignedTransaction& tx, const NondeterministicResults& nres)
{
    assert_txs_not_merged();
    auto& local_trie = cache.get(txs);
    local_trie.template insert<ResultListInsertFn, TxSetEntry>(
        hash, TxSetEntry(tx, xdr::xvector<NondeterministicResults>{nres}));
}

void
TxSet::finalize()
{
    assert_txs_not_merged();
    txs.template batch_merge_in<ResultListMergeFn>(cache);
    txs_merged = true;
}

Block 
TxSet::serialize_block() const
{
    Block out;
    assert_txs_merged();

    txs.accumulate_values_parallel(out.transactions);

    return out;
}

Hash
TxSet::hash()
{
    assert_txs_merged();
    Hash out;
    txs.hash(out);

    return out;
}

} // namespace scs