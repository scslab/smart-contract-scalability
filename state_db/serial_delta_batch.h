#pragma once

#include "state_db/delta_vec.h"

#include "xdr/storage.h"
#include "xdr/types.h"

#include "state_db/delta_batch_value.h"

#include "mtt/trie/recycling_impl/trie.h"

#include "mtt/utils/non_movable.h"

#include <map>
#include <vector>

namespace scs {

class DeltaBatch;
struct StorageProxyValue;

class SerialDeltaBatch : public utils::NonMovableOrCopyable
{
    using value_t = DeltaBatchValue;
    using trie_prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey)>;

    using map_t = trie::
        SerialRecyclingTrie<value_t, trie_prefix_t, DeltaBatchValueMetadata>;

    map_t& deltas;

  public:
    SerialDeltaBatch(map_t& serial_trie);

    void add_deltas(const AddressAndKey& key, StorageProxyValue&& v, const Hash& tx_hash);
};

} // namespace scs
