#include "state_db/serial_delta_batch.h"

#include "state_db/delta_batch.h"

#include "storage_proxy/storage_proxy_value.h"

namespace scs {

SerialDeltaBatch::SerialDeltaBatch(map_t& serial_trie)
    : deltas(serial_trie)
{}

struct insert_t
{
    StorageProxyValue proxy_value;
    Hash const& tx_hash;
};

struct AppendInsertFn
{
    using base_value_type = DeltaBatchValue;
    using prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey)>;

    static base_value_type new_value(const prefix_t& prefix)
    {
        return base_value_type();
    }

    static void value_insert(base_value_type& main_value, insert_t&& v)
    {
        DeltaVector dv;
        for (auto& d : v.proxy_value.vec)
        {
            dv.add_delta(std::move(d), Hash(v.tx_hash));
        }
        main_value.vectors.back().add(std::move(dv));
        main_value.add_tc(v.proxy_value.get_tc());
    }
};

void
SerialDeltaBatch::add_deltas(const AddressAndKey& key, StorageProxyValue&& v, const Hash& tx_hash)
{
    if (v.vec.size() == 0) {
        return;
    }

    deltas.template insert<AppendInsertFn>((key), insert_t{
        .proxy_value = std::move(v),
        .tx_hash = tx_hash
    });
}

} // namespace scs
