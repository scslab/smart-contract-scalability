#pragma once

#include "state_db/delta_vec.h"
#include "object/object_mutator.h"

#include "xdr/storage.h"
#include "xdr/types.h"

#include "state_db/delta_batch_value.h"

#include "mtt/trie/recycling_impl/trie.h"

#include <map>
#include <vector>

namespace scs
{

class DeltaBatch;
struct StorageProxyValue;

class SerialDeltaBatch
{
	// accumulator for all deltas in a block.
	// ultimately will have threadlocal cache of these
	// Each vec is kept in sort order by priority

	/*struct value_t
	{
		DeltaVector vec;
		//std::optional<StorageObject> base_obj;

		value_t(std::optional<StorageObject> const& obj)
			: vec()
			, base_obj(obj)
			{}
	}; */

	using value_t = DeltaBatchValue;
	using trie_prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey)>;

	using map_t = trie::SerialRecyclingTrie<value_t, trie_prefix_t, DeltaBatchValueMetadata>;

	map_t& deltas;

public:

	SerialDeltaBatch(map_t& serial_trie);

	SerialDeltaBatch(const SerialDeltaBatch& other) = delete;
	SerialDeltaBatch(SerialDeltaBatch&& other) = default;

	void add_deltas(const AddressAndKey& key, StorageProxyValue&& v);
};

} /* scs */
