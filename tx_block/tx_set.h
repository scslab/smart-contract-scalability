#pragma once

#include "mtt/trie/prefix.h"
#include "mtt/trie/recycling_impl/trie.h"

#include "mtt/utils/threadlocal_cache.h"

#include "xdr/transaction.h"
#include "xdr/types.h"

#include <xdrpp/marshal.h>

namespace scs {

class TxSet
{
    static std::vector<uint8_t> serialize(const Transaction& v)
    {
        return xdr::xdr_to_opaque(v);
    }

    using value_t = trie::XdrTypeWrapper<Transaction, &serialize>;

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
    void add_transaction(const Hash& hash, const Transaction& tx);

    void finalize();

    const map_t& get_txs() const
    {
        assert_txs_merged();
        return txs;
    }

    // for testing

    bool contains_tx(const Hash& hash) const
    {
        return txs.get_value(hash) != nullptr;
    }
};

} // namespace scs