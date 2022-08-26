#include "state_db/serial_delta_batch.h"

#include "state_db/delta_batch.h"

#include "storage_proxy/storage_proxy_value.h"

namespace scs {

SerialDeltaBatch::SerialDeltaBatch(map_t& serial_trie)
    : deltas(serial_trie)
{}

struct AppendInsertFn
{
    using base_value_type = DeltaBatchValue;
    using prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey)>;

    static base_value_type new_value(const prefix_t& prefix)
    {
        return base_value_type();
    }

    static void value_insert(base_value_type& main_value, StorageProxyValue&& v)
    {
        main_value.vectors.back().add(std::move(v.vec));
        main_value.add_tc(v.get_tc());
    }
};

void
SerialDeltaBatch::add_deltas(const AddressAndKey& key, StorageProxyValue&& v)
{
    if (v.vec.size() == 0) {
        return;
    }

    deltas.template insert<AppendInsertFn>((key), std::move(v));
}

} // namespace scs
