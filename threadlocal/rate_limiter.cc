#include "threadlocal/rate_limiter.h"

namespace scs {

void
RateLimiter::join_threads()
{
    shutdown();
    max_active_threads = 0;

    wake_cond.notify_all();

    auto done = [&]() -> bool {
        return active_threads.load(std::memory_order_relaxed) == 0;
    };

    std::unique_lock lock(mtx);

    if (!done()) {
        join_cond.wait(lock, done);
    }
}

} // namespace scs
