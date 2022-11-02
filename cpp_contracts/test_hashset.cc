#include "sdk/hashset.h"
#include "sdk/calldata.h"
#include "sdk/invoke.h"
#include "sdk/crypto.h"

struct calldata_0
{
	sdk::StorageKey key;
	uint64_t value;
};

EXPORT("pub00000000")
hashset_insert()
{
	auto calldata = sdk::get_calldata<calldata_0>();
	auto hash = sdk::hash(reinterpret_cast<uint8_t*>(&(calldata.value)), sizeof(uint64_t));

	sdk::hashset_insert(calldata.key, hash, 0);
}

