#include "config/print_configs.h"
#include "config.h"

#include "config/static_constants.h"

#include <cstdio>
#include <thread>
#include <cinttypes>

namespace scs {

void
print_configs()
{
    std::printf("=========== scs configs  ==========\n");
    std::printf("USE_RPC      = %d\n", USE_RPC);
    std::printf("HW THREADS   = %u\n", std::thread::hardware_concurrency());
    std::printf("TLCACHE_SIZE = %" PRIu32 "\n", TLCACHE_SIZE);
}

} // namespace scs
