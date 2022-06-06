#pragma once

#include "xdr/storage.h"
#include "xdr/types.h"

#include <optional>
#include <map>

namespace scs
{

class SerialDeltaBatch;
class DeltaBatch;

class StateDB
{
	std::map<AddressAndKey, StorageObject> state_db;

public:

	std::optional<StorageObject> 
	get(const AddressAndKey& a) const;

	void 
	populate_delta_batch(SerialDeltaBatch& delta_batch) const;

	void 
	apply_delta_batch(DeltaBatch const& delta_batch);
};


} /* scs */
