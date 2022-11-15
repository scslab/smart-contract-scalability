#include "sdk/alloc.h"
#include "sdk/replay_cache.h"
#include "sdk/semaphore.h"
#include "sdk/constexpr.h"
#include "sdk/raw_memory.h"
#include "sdk/invoke.h"
#include "sdk/calldata.h"
#include "sdk/auth_singlekey.h"
#include "sdk/log.h"
#include "sdk/flag.h"
#include "sdk/witness.h"
#include "sdk/crypto.h"
#include "sdk/general_storage.h"

#include <cstdint>

constexpr static sdk::StorageKey addr = sdk::make_static_key(0, 2);

sdk::Hash 
get_hash(uint64_t val)
{
	return sdk::hash(val);
}

EXPORT("pub00000000")
test_see_hs_writes_from_empty()
{
	sdk::print("start hashset manipulation tests phase 0");
	if (sdk::has_key(addr))
	{
		abort();
	}

	sdk::hashset_insert(addr, get_hash(0), 0);

	sdk::print("check size");
	assert(sdk::hashset_get_size(addr) == 1);
	sdk::print("check max size");
	assert(sdk::hashset_get_max_size(addr) == 64);

	sdk::hashset_insert(addr, get_hash(1), 1);

	assert(sdk::hashset_get_size(addr) == 2);

	sdk::hashset_insert(addr, get_hash(100), 0);
	sdk::hashset_insert(addr, get_hash(2), 2);
	sdk::hashset_insert(addr, get_hash(21), 2);

	assert(sdk::hashset_get_index_of(addr, 1) == 2);
	assert(sdk::hashset_get_index_of(addr, 0) == 3);

	assert(sdk::hashset_get_index(addr, 2).second == get_hash(1));
}
