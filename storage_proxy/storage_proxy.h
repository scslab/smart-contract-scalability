#pragma once

#include "xdr/storage.h"
#include "xdr/types.h"

#include "storage_proxy/storage_proxy_value.h"

#include <cstdint>
#include <vector>
#include <map>

namespace scs
{
class StateDB;
class ModifiedKeysList;

class StorageProxy
{
	StateDB& state_db;
	ModifiedKeysList& keys;

	using value_t = StorageProxyValue;

	std::map<AddressAndKey, value_t> cache;

	value_t& get_local(AddressAndKey const& key);

	bool committed_local_values = false;

public:

	using delta_identifier_t = Hash; // was DeltaPriority

	StorageProxy(StateDB& state_db, ModifiedKeysList& keys);

	std::optional<StorageObject>
	get(AddressAndKey const& key);

	void
	raw_memory_write(AddressAndKey const& key, xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& bytes, delta_identifier_t id);

	void
	nonnegative_int64_set_add(AddressAndKey const& key, int64_t set_value, int64_t delta, delta_identifier_t id);

	void
	delete_object_last(AddressAndKey const& key, delta_identifier_t id);

	bool
	__attribute__((warn_unused_result))
	 push_deltas_to_statedb();
};

} /* scs */
