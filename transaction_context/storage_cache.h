#pragma once

#include "xdr/storage.h"
#include "xdr/types.h"

#include <cstdint>
#include <vector>
#include <map>

namespace scs
{
class StateDB;

class StorageCache
{

	StateDB const& state_db;

	std::map<AddressAndKey, StorageObject> cache;

public:

	StorageCache(const StateDB& state_db);

	StorageObject const& get(AddressAndKey const& key);
};

} /* scs */
