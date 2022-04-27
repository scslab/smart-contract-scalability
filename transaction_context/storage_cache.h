#pragma once

#include "xdr/storage.h"

#include <cstdint>
#include <vector>
#include <map>

namespace scs
{
class StateDB;

class StorageCache
{

	StateDB const& state_db;

	std::map<std::vector<uint8_t>, StorageObject> cache;

public:

	StorageCache(const StateDB& state_db);

	StorageObject const& get(std::vector<uint8_t> const& key);
};

} /* scs */
