#pragma once

#include "xdr/storage.h"
#include "xdr/types.h"

#include "object/delta_applicator.h"
#include "state_db/delta_vec.h"

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

	struct value_t
	{
		DeltaApplicator applicator;
		DeltaVector vec;
	};

	std::map<AddressAndKey, value_t> cache;

	// may not be empty -- can accumulate deltas from 
	// an entire thread's worth of work
	SerialDeltaBatch& local_delta_batch;

	value_t& get_local(AddressAndKey const& key);

	bool committed_local_values = false;

public:

	StorageProxy(const StateDB& state_db, SerialDeltaBatch& local_delta_batch);

	std::optional<StorageObject>
	get(AddressAndKey const& key);

	void
	raw_memory_write(AddressAndKey const& key, xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& bytes, DeltaPriority&& priority);

	void
	nonnegative_int64_set_add(AddressAndKey const& key, int64_t set_value, int64_t delta, DeltaPriority&& priority);

	void
	delete_object_last(AddressAndKey const& key, DeltaPriority&& priority);

	void
	delete_object_first(AddressAndKey const& key, DeltaPriority&& priority);

	void push_deltas_to_batch();
};

} /* scs */
