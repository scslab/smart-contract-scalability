#pragma once

#include "xdr/storage.h"
#include "xdr/types.h"

#include <map>

namespace scs
{

class SerialDeltaBatch;

class StateDB
{
	std::map<AddressAndKey, StorageObject> state_db;

public:

	void populate_delta_batch(SerialDeltaBatch& delta_batch) const;

	void apply_delta_batch(SerialDeltaBatch const& delta_batch);
};


} /* scs */
