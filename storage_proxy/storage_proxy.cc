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
#include "state_db/state_db_v2.h"
#include "state_db/sisyphus_state_db.h"
#include "state_db/modified_keys_list.h"
#include "state_db/typed_modification_index.h"
#include "state_db/groundhog_persistent_state_db.h"

#include "storage_proxy/transaction_rewind.h"

#include "transaction_context/error.h"

#include "object/make_delta.h"

#include "debug/debug_utils.h"

namespace scs
{

template class StorageProxy<StateDB>;
template class StorageProxy<StateDBv2>;
template class StorageProxy<SisyphusStateDB>;
template class StorageProxy<GroundhogPersistentStateDB>;

#define PROXY_TEMPLATE template<typename StateDB_t>
#define PROXY_DECL StorageProxy<StateDB_t>

PROXY_TEMPLATE
PROXY_DECL::StorageProxy(StateDB_t& state_db)
	: state_db(state_db)
	, cache()
	{}

PROXY_TEMPLATE
StorageProxy<StateDB_t>::value_t& 
StorageProxy<StateDB_t>::get_local(AddressAndKey const& key) const
{
	auto it = cache.find(key);
	if (it == cache.end())
	{
		auto res = state_db.get_committed_value(key);
		it = cache.emplace(key, res).first;
	}
	return it->second;
}

PROXY_TEMPLATE
std::optional<StorageObject>
PROXY_DECL::get(AddressAndKey const& key) const
{
	return get_local(key).applicator.get();
}

PROXY_TEMPLATE
bool
PROXY_DECL::raw_memory_write(
	AddressAndKey const& key, 
	xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& bytes)
{
	auto& v = get_local(key);

	auto delta = make_raw_memory_write(std::move(bytes));

	return v.applicator.try_apply(delta);
}

PROXY_TEMPLATE
bool
PROXY_DECL::nonnegative_int64_set_add(
	AddressAndKey const& key, 
	int64_t set_value, 
	int64_t delta_value)
{
	auto& v = get_local(key);
	auto delta = make_nonnegative_int64_set_add(set_value, delta_value);

	return v.applicator.try_apply(delta);
}

PROXY_TEMPLATE
bool 
PROXY_DECL::nonnegative_int64_add(AddressAndKey const& key, int64_t delta_value)
{
	auto& v = get_local(key);
	auto base_value = v.applicator.get_base_nnint64_set_value();
	if (!base_value.has_value())
	{
		return false;
	}
	auto delta = make_nonnegative_int64_set_add(*base_value, delta_value);

	return v.applicator.try_apply(delta);
}

PROXY_TEMPLATE
bool
PROXY_DECL::delete_object_last(AddressAndKey const& key)
{
	auto& v = get_local(key);

	auto delta = make_delete_last();

	return v.applicator.try_apply(delta);
}

PROXY_TEMPLATE
bool
PROXY_DECL::hashset_insert(AddressAndKey const& key, Hash const& h, uint64_t threshold)
{
	auto& v = get_local(key);

	auto delta = make_hash_set_insert(h, threshold);

	return v.applicator.try_apply(delta);
}

PROXY_TEMPLATE
bool
PROXY_DECL::hashset_increase_limit(AddressAndKey const& key, uint32_t limit)
{
	auto& v = get_local(key);

	static_assert(MAX_HASH_SET_SIZE <= UINT16_MAX, "uint16 overflow otherwise");

	if (limit > UINT16_MAX)
	{
		return false;
	}

	auto delta = make_hash_set_increase_limit(limit);

	return v.applicator.try_apply(delta);
}

PROXY_TEMPLATE
bool
PROXY_DECL::hashset_clear(AddressAndKey const& key, uint64_t threshold)
{
	auto& v = get_local(key);

	auto delta = make_hash_set_clear(threshold);

	return v.applicator.try_apply(delta);
}

PROXY_TEMPLATE
bool
PROXY_DECL::asset_add(AddressAndKey const& key, int64_t d)
{
	auto& v = get_local(key);

	auto delta = make_asset_add(d);

	return v.applicator.try_apply(delta);
}

PROXY_TEMPLATE
bool 
PROXY_DECL::push_deltas_to_statedb(TransactionRewind& rewind) const
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

PROXY_TEMPLATE
void 
PROXY_DECL::log_modified_keys(ModifiedKeysList& keys, const Hash&)
{
	assert_not_committed_local_values();

	for (auto const& [k, _] : cache)
	{
		keys.log_key(k);
	}

	committed_local_values = true;
}

PROXY_TEMPLATE
void 
PROXY_DECL::log_modified_keys(TypedModificationIndex& keys, const Hash& src_hash)
{
	assert_not_committed_local_values();

	for (auto const& [k, v] : cache)
	{
		auto deltas = v.applicator.get_deltas();
		for (auto const& delta : deltas)
		{
			keys.log_modification(k, delta, src_hash);
		}
	}

	committed_local_values = true;
}

PROXY_TEMPLATE
void 
PROXY_DECL::assert_not_committed_local_values() const
{
	if (committed_local_values)
	{
		throw std::runtime_error("double commit");
	}
}

#undef PROXY_DECL
#undef PROXY_TEMPLATE

} /* scs */
