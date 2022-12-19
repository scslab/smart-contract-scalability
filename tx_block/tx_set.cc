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

struct ResultListInsertFn //: public trie::GenericInsertFn<TxSet::value_t>
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
 /*
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
 */

static TxSetEntry const& get_txset_entry(const LockableTxSetEntry& entry)
{
    return entry.get();
}

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
   // txs.template batch_merge_in<ResultListMergeFn>(cache);
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
