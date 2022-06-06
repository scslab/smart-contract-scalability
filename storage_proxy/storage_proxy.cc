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


std::optional<StorageObject> const& 
StorageProxy::get(AddressAndKey const& key)
{
	auto it = cache.find(key);

	if (it == cache.end())
	{
		auto const& res = state_db.get(key);
		cache.emplace(key, res);
		return res;
	}

	return it -> second.get();
}

void
StorageProxy::raw_memory_write(
	AddressAndKey const& key, 
	xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& bytes, 
	DeltaPriority&& priority)
{
	auto it = cache.find(key);
	if (it == cache.end())
	{
		auto res = state_db.get(key);
		it = cache.emplace(key, res).first;
	}

	auto delta = make_raw_memory_write(std::move(bytes));

	if (!(it -> second).try_apply(delta))
	{
		throw wasm_api::HostError("failed to apply raw_memory_write");
	}

	local_delta_batch.add_delta(key, std::move(delta), std::move(priority));
}




} /* scs */
