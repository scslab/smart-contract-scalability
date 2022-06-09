#include "storage_proxy/storage_proxy.h"

#include "state_db/state_db.h"

#include <wasm_api/error.h>

#include "object/make_delta.h"
#include "state_db/serial_delta_batch.h"

namespace scs
{

StorageProxy::StorageProxy(const StateDB& state_db, SerialDeltaBatch& local_delta_batch)
	: state_db(state_db)
	, cache()
	, local_delta_batch(local_delta_batch)
	{}

StorageProxy::value_t& 
StorageProxy::get_local(AddressAndKey const& key)
{
	auto it = cache.find(key);
	if (it == cache.end())
	{
		auto res = state_db.get(key);
		it = cache.emplace(key, res).first;
	}
	return it->second;
}

std::optional<StorageObject> const& 
StorageProxy::get(AddressAndKey const& key)
{
	return get_local(key).applicator.get();
}

void
StorageProxy::raw_memory_write(
	AddressAndKey const& key, 
	xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& bytes, 
	DeltaPriority&& priority)
{
	auto& v = get_local(key);

	auto delta = make_raw_memory_write(std::move(bytes));

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply raw_memory_write");
	}

	v.vec.add_delta(std::move(delta), std::move(priority));
}


void
StorageProxy::delete_object_last(AddressAndKey const& key, DeltaPriority&& priority)
{
	auto& v = get_local(key);

	auto delta = make_delete_last();

	if (!v.applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply delete");
	}
	v.vec.add_delta(std::move(delta), std::move(priority));
}

void 
StorageProxy::push_deltas_to_batch()
{
	if (committed_local_values)
	{
		throw std::runtime_error("double push to batch");
	}

	for (auto& [k, v] : cache)
	{
		local_delta_batch.add_deltas(k, std::move(v.vec));
	}

	committed_local_values = true;
}



} /* scs */
