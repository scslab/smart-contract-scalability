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

	v.vec.add_delta(std::move(delta), std::move(id));
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

	v.vec.add_delta(std::move(delta), std::move(id));
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
	v.vec.add_delta(std::move(delta), std::move(id));
}


bool 
StorageProxy::push_deltas_to_statedb(TransactionRewind& rewind)
{
	if (committed_local_values)
	{
		throw std::runtime_error("double push to batch");
	}

	for (auto const& [k, v] : cache)
	{
		auto const& dv = v.vec;
		auto const& deltas = dv.get();
		for (auto const& delta : deltas)
		{
			auto res = state_db.try_apply_delta(k, delta.first);
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
