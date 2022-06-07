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
	return get_local(key).get();
}

void
StorageProxy::raw_memory_write(
	AddressAndKey const& key, 
	xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& bytes, 
	DeltaPriority&& priority)
{
	auto& applicator = get_local(key);

	auto delta = make_raw_memory_write(std::move(bytes));

	if (!applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply raw_memory_write");
	}

	local_delta_batch.add_delta(key, std::move(delta), std::move(priority));
}


void
StorageProxy::delete_object(AddressAndKey const& key, DeltaPriority&& priority)
{
	auto& applicator = get_local(key);

	auto delta = make_delete();

	if (!applicator.try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply delete");
	}
	local_delta_batch.add_delta(key, std::move(delta), std::move(priority));
}



} /* scs */
