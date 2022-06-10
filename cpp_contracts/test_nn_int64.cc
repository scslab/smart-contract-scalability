#include "sdk/calldata.h"
#include "sdk/log.h"
#include "sdk/raw_memory.h"
#include "sdk/invoke.h"
#include "sdk/nonnegative_int64.h"

struct calldata_0 {
	sdk::StorageKey key;
	int64_t delta;
};

EXPORT("pub00000000")
call_int64_add()
{
	auto calldata = sdk::get_calldata<calldata_0>();

	sdk::int64_add(calldata.key, calldata.delta);
}

struct calldata_1 {
	sdk::StorageKey key;
	int64_t value;
	int64_t delta;
};

EXPORT("pub01000000")
call_int64_set_add()
{
	auto calldata = sdk::get_calldata<calldata_1>();

	sdk::int64_set_add(calldata.key, calldata.value, calldata.delta);
}
