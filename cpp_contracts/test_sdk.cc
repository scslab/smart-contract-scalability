#include "sdk/replay_cache.h"
#include "sdk/semaphore.h"
#include "sdk/constexpr.h"
#include "sdk/calldata.h"

struct replay_calldata {
	uint64_t expiry_time;
};

EXPORT("pub00000000")
replay()
{
	auto calldata = sdk::get_calldata<replay_calldata>();
	sdk::record_self_replay(calldata.expiry_time);
}

static sdk::Semaphore s(sdk::make_static_key(0, 1));
static sdk::Semaphore<2> s2 (sdk::make_static_key(1, 1));

EXPORT("pub01000000")
semaphore()
{
	s.acquire();
}

EXPORT("pub02000000")
semaphore2()
{
	s2.acquire();
}

EXPORT("pub03000000")
transientSemaphore()
{
	sdk::TransientSemaphore (sdk::make_static_key(2, 1)).acquire();
}

