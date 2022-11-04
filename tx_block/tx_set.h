#pragma once

#include "mtt/trie/prefix.h"
#include "mtt/trie/recycling_impl/trie.h"

#include <utils/threadlocal_cache.h>

#include "xdr/transaction.h"
#include "xdr/types.h"

#include <xdrpp/marshal.h>

namespace scs {

// TODO tag every transaction with its results (logs)

class TxSet
{
    static std::vector<uint8_t> serialize(const TxSetEntry& v)
    {
        return xdr::xdr_to_opaque(v);
    }

    friend struct MultiplicityAddInsertFn;
    friend struct MultiplicityAddMergeFn;

    using value_t = trie::XdrTypeWrapper<TxSetEntry, &serialize>;

    using trie_prefix_t = trie::ByteArrayPrefix<sizeof(Hash)>;

    using metadata_t = void;

    using map_t = trie::RecyclingTrie<value_t, trie_prefix_t, metadata_t>;

    map_t txs;

    using serial_trie_t = map_t::serial_trie_t;

    using cache_t = utils::ThreadlocalCache<serial_trie_t>;

    cache_t cache;

    bool txs_merged = false;

    void assert_txs_merged() const;
    void assert_txs_not_merged() const;

  public:
    void add_transaction(const Hash& hash, const SignedTransaction& tx);

    void finalize();

    const map_t& get_txs() const
    {
        assert_txs_merged();
        return txs;
    }

    Hash hash();

    // for testing

    uint32_t contains_tx(const Hash& hash) const
    {
        auto const* r = txs.get_value(hash);
        if (r == nullptr)
        {
            return 0;
        }
        return r -> multiplicity;
    }
};

} // namespace scs