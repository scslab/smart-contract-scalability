#include "sdk/calldata.h"
#include "sdk/log.h"
#include "sdk/raw_memory.h"
#include "sdk/invoke.h"

struct calldata_0 {
	sdk::StorageKey key;
};

EXPORT("pub00000000")
log_key()
{
	auto calldata = sdk::get_calldata<calldata_0>();

	uint64_t stored_value = sdk::get_raw_memory<uint64_t>(calldata.key);

	sdk::log(stored_value);
}

struct calldata_1 {
	sdk::StorageKey key;
	uint64_t value;
};

EXPORT("pub01000000")
set_key()
{
	auto calldata = sdk::get_calldata<calldata_1>();

	sdk::set_raw_memory<uint64_t>(calldata.key, calldata.value);
}

EXPORT("pub02000000")
check_self_writes_visible()
{
	sdk::StorageKey key;

	calldata_0 c0 {.key = key};
	calldata_1 c1 {.key = key, .value = 0x1234};
	calldata_1 c1_1 {.key = key, .value = 0xAABB};

	auto self = sdk::get_self();

	sdk::invoke(self, 1, c1);
	sdk::invoke(self, 0, c0);
	sdk::invoke(self, 1, c1_1);
	sdk::invoke(self, 0, c0);
}