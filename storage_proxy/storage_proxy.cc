#include "storage_proxy/storage_proxy.h"

#include "state_db/state_db.h"
#include "state_db/modified_keys_list.h"

#include "storage_proxy/transaction_rewind.h"

#include <wasm_api/error.h>

#include "object/make_delta.h"

namespace scs
{

StorageProxy::StorageProxy(StateDB& state_db, ModifiedKeysList& keys)
	: state_db(state_db)
	, keys(keys)
	, cache()
	{}

StorageProxy::value_t& 
StorageProxy::get_local(AddressAndKey const& key)
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
StorageProxy::get(AddressAndKey const& key)
{
	return get_local(key).applicator.get();
}

void
StorageProxy::raw_memory_write(
	AddressAndKey const& key, 
	xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& bytes, 
	delta_identifier_t id)
{
	auto& v = get_local(key);

	auto delta = make_raw_memory_write(std::move(bytes));

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply raw_memory_write");
	}

	//v.vec.emplace_back(std::move(delta));

	//v.vec.add_delta(std::move(delta), std::move(id));
}

void
StorageProxy::nonnegative_int64_set_add(
	AddressAndKey const& key, 
	int64_t set_value, 
	int64_t delta_value, 
	delta_identifier_t id)
{
	auto& v = get_local(key);
	auto delta = make_nonnegative_int64_set_add(set_value, delta_value);

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply nonnegative_int64_set_add");
	}
}

void 
StorageProxy::nonnegative_int64_add(AddressAndKey const& key, int64_t delta_value, delta_identifier_t id)
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
StorageProxy::delete_object_last(AddressAndKey const& key, delta_identifier_t id)
{
	auto& v = get_local(key);

	auto delta = make_delete_last();

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply delete_last");
	}
	//v.vec.emplace_back(std::move(delta));
	//v.vec.add_delta(std::move(delta), std::move(id));
}

void
StorageProxy::hashset_insert(AddressAndKey const& key, Hash const& h, uint64_t threshold, delta_identifier_t id)
{
	auto& v = get_local(key);

	auto delta = make_hash_set_insert(h, threshold);

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply hashset insert");
	}
}

void
StorageProxy::hashset_increase_limit(AddressAndKey const& key, uint32_t limit, delta_identifier_t id)
{
	auto& v = get_local(key);

	static_assert(MAX_HASH_SET_SIZE <= UINT16_MAX, "uint16 overflow otherwise");

	if (limit >= UINT16_MAX)
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
StorageProxy::hashset_clear(AddressAndKey const& key, uint64_t threshold, delta_identifier_t id)
{
	auto& v = get_local(key);

	auto delta = make_hash_set_clear(threshold);

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply hashset clear");
	}
}


bool 
StorageProxy::push_deltas_to_statedb(TransactionRewind& rewind) const
{
	if (committed_local_values)
	{
		throw std::runtime_error("double push to batch");
	}

	for (auto const& [k, v] : cache)
	{

		auto deltas = v.applicator.get_deltas();
		//auto const& dv = v.vec;
		//auto const& deltas = dv.get();
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

void StorageProxy::commit()
{
	for (auto const& [k, _] : cache)
	{
		keys.log_key(k);
	}

	committed_local_values = true;
}

/*
void 
StorageProxy::push_deltas_to_batch(SerialDeltaBatch& local_delta_batch)
{
	if (committed_local_values)
	{
		throw std::runtime_error("double push to batch");
	}

	for (auto& [k, v] : cache)
	{
		local_delta_batch.add_deltas(k, std::move(v));
	}

	committed_local_values = true;
} */



} /* scs */
