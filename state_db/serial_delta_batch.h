#pragma once

#include "state_db/delta_vec.h"
#include "state_db/object_mutator.h"

#include "xdr/storage.h"
#include "xdr/types.h"

#include <map>
#include <vector>

namespace scs
{

class SerialDeltaBatch
{
	// accumulator for all deltas in a block.
	// ultimately will have threadlocal cache of these
	// Each vec is kept in sort order by priority

	using value_t = std::pair<DeltaVector, ObjectMutator>;

	using map_t = std::map<AddressAndKey, value_t>;

	map_t deltas;

	void add_delta(const Address& addr, const InvariantKey& key, StorageDelta&& delta, DeltaPriority&& priority);

public:

	void add_delta_raw_memory_write(const Address& addr, const InvariantKey& key, xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& data, DeltaPriority&& priority);

	map_t& get_delta_map() {
		return deltas;
	}

	const map_t& get_delta_map() const {
		return deltas;
	}
};

} /* scs */
