#pragma once

/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "xdr/storage.h"
#include "xdr/types.h"

#include "storage_proxy/storage_proxy_value.h"

#include <cstdint>
#include <vector>
#include <map>

namespace scs
{

class TransactionRewind;
class TypedModificationIndex;

template<typename StateDB_t>
class StorageProxy
{
	StateDB_t& state_db;

	using value_t = StorageProxyValue;

	mutable std::map<AddressAndKey, value_t> cache;

	value_t& get_local(AddressAndKey const& key) const;

	bool committed_local_values = false;

	void assert_not_committed_local_values() const;

public:

	StorageProxy(StateDB_t& state_db);

	std::optional<StorageObject>
	get(AddressAndKey const& key) const;

	void
	raw_memory_write(AddressAndKey const& key, xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& bytes, uint64_t priority);

	void
	nonnegative_int64_set_add(AddressAndKey const& key, int64_t set_value, int64_t delta, uint64_t priority);

	void
	nonnegative_int64_add(AddressAndKey const& key, int64_t delta, uint64_t priority);

	void
	delete_object_last(AddressAndKey const& key);

	void
	hashset_insert(AddressAndKey const& key, Hash const& h, uint64_t threshold, uint64_t priority);

	void
	hashset_increase_limit(AddressAndKey const& key, uint32_t limit);

	void
	hashset_clear(AddressAndKey const& key, uint64_t threshold);

	void
	asset_add(AddressAndKey const& key, int64_t d, uint64_t priority);

	void log_modified_keys(TypedModificationIndex& keys, const Hash& src_hash);
};

} /* scs */
