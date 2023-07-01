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

#include "storage_proxy/storage_proxy.h"

#include "state_db/state_db.h"
#include "state_db/modified_keys_list.h"

#include "storage_proxy/transaction_rewind.h"

#include <wasm_api/error.h>

#include "object/make_delta.h"

#include "debug/debug_utils.h"

namespace scs
{

StorageProxy::StorageProxy(StateDB& state_db)
	: state_db(state_db)
	, cache()
	{}

StorageProxy::value_t& 
StorageProxy::get_local(AddressAndKey const& key) const
{
	auto it = cache.find(key);
	if (it == cache.end())
	{
		auto res = state_db.get_committed_value(key);
		it = cache.emplace(key, res).first;
	}
	return it->second;
}

std::optional<StorageObject>
StorageProxy::get(AddressAndKey const& key) const
{
	return get_local(key).applicator.get();
}

void
StorageProxy::raw_memory_write(
	AddressAndKey const& key, 
	xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& bytes)
{
	auto& v = get_local(key);

	auto delta = make_raw_memory_write(std::move(bytes));

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply raw_memory_write");
	}
}

void
StorageProxy::nonnegative_int64_set_add(
	AddressAndKey const& key, 
	int64_t set_value, 
	int64_t delta_value)
{
	auto& v = get_local(key);
	auto delta = make_nonnegative_int64_set_add(set_value, delta_value);

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply nonnegative_int64_set_add");
	}
}

void 
StorageProxy::nonnegative_int64_add(AddressAndKey const& key, int64_t delta_value)
{
	auto& v = get_local(key);
	auto base_value = v.applicator.get_base_nnint64_set_value();
	if (!base_value.has_value())
	{
		throw wasm_api::HostError("type mismatch in nonnegative_int64_add");
	}
	auto delta = make_nonnegative_int64_set_add(*base_value, delta_value);

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply nonnegative_int64_add");
	}
}

void
StorageProxy::delete_object_last(AddressAndKey const& key)
{
	auto& v = get_local(key);

	auto delta = make_delete_last();

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply delete_last");
	}
}

void
StorageProxy::hashset_insert(AddressAndKey const& key, Hash const& h, uint64_t threshold)
{
	auto& v = get_local(key);

	auto delta = make_hash_set_insert(h, threshold);

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply hashset insert");
	}
}

void
StorageProxy::hashset_increase_limit(AddressAndKey const& key, uint32_t limit)
{
	auto& v = get_local(key);

	static_assert(MAX_HASH_SET_SIZE <= UINT16_MAX, "uint16 overflow otherwise");

	if (limit > UINT16_MAX)
	{
		throw wasm_api::HostError("limit increase too large");
	}

	auto delta = make_hash_set_increase_limit(limit);

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply hashset limit increase");
	}
}

void
StorageProxy::hashset_clear(AddressAndKey const& key, uint64_t threshold)
{
	auto& v = get_local(key);

	auto delta = make_hash_set_clear(threshold);

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply hashset clear");
	}
}

void
StorageProxy::asset_add(AddressAndKey const& key, int64_t d)
{
	auto& v = get_local(key);

	auto delta = make_asset_add(d);

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply asset add");
	}
}


bool 
StorageProxy::push_deltas_to_statedb(TransactionRewind& rewind) const
{
	assert_not_committed_local_values();

	for (auto const& [k, v] : cache)
	{
		auto deltas = v.applicator.get_deltas();
		for (auto const& delta : deltas)
		{
			auto res = state_db.try_apply_delta(k, delta);
			if (res)
			{
				rewind.add(std::move(*res));
			} 
			else
			{
				return false;
			}
		}
	}
	return true;
}

void StorageProxy::log_modified_keys(ModifiedKeysList& keys)
{
	assert_not_committed_local_values();

	for (auto const& [k, _] : cache)
	{
		keys.log_key(k);
	}

	committed_local_values = true;
}

void 
StorageProxy::assert_not_committed_local_values() const
{
	if (committed_local_values)
	{
		throw std::runtime_error("double commit");
	}
}

} /* scs */
