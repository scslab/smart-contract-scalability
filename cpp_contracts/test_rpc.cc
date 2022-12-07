#include "sdk/alloc.h"
#include "sdk/calldata.h"
#include "sdk/crypto.h"
#include "sdk/invoke.h"
#include "sdk/log.h"
#include "sdk/nonnegative_int64.h"
#include "sdk/raw_memory.h"
#include "sdk/types.h"
#include "sdk/constexpr.h"
#include "sdk/rpc.h"
#include "sdk/semaphore.h"

uint64_t dispatch_call(sdk::Hash const& addr, uint64_t input)
{
	return sdk::external_call<uint64_t, uint64_t>(addr, input);
}

struct calldata {
	sdk::Hash addr;
	uint64_t call;
};

EXPORT("pub00000000")
dispatch()
{
	auto c = sdk::get_calldata<calldata>();

	uint64_t res = dispatch_call(c.addr, c.call);

	if (res != c.call) {
		abort();
	}

	sdk::log(res);
}

