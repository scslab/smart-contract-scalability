#include "sdk/replay_cache.h"
#include "sdk/semaphore.h"
#include "sdk/constexpr.h"

EXPORT("pub00000000")
replay()
{
	sdk::record_self_replay();
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

