#include "config/print_configs.h"
#include "config.h"

#include <cstdio>
#include <thread>

namespace scs {

void
print_configs()
{
    std::printf("=========== SCS CONFIGS ===========\n");
    std::printf("USE_RPC    = %d\n", USE_RPC);
    std::printf("HW THREADS = %u\n", std::thread::hardware_concurrency());
}

} // namespace scs