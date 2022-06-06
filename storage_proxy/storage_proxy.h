#pragma once

#include "xdr/storage.h"
#include "xdr/types.h"

#include "object/delta_applicator.h"

#include <cstdint>
#include <vector>
#include <map>

namespace scs
{
class StateDB;
class SerialDeltaBatch;

class StorageProxy
{
	StateDB const& state_db;

	using map_value_t = DeltaApplicator;

	std::map<AddressAndKey, map_value_t> cache;

	// may not be empty -- can accumulate deltas from 
	// an entire thread's worth of work
	SerialDeltaBatch& local_delta_batch;

public:

	StorageProxy(const StateDB& state_db, SerialDeltaBatch& local_delta_batch);

	std::optional<StorageObject> const& 
	get(AddressAndKey const& key);

	void
	raw_memory_write(AddressAndKey const& key, xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& bytes, DeltaPriority&& priority);
};

} /* scs */
