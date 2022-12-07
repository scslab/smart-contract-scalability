#include "threadlocal/rate_limiter.h"

namespace scs {

bool
RateLimiter::wait_for_opening()
{
    auto done = [&]() {
        return _shutdown.load(std::memory_order_relaxed)
               || (active_threads < max_active_threads);
    };

    std::unique_lock lock(mtx);

    auto claim_or_shutdown = [&]() -> bool {
        if (_shutdown.load(std::memory_order_relaxed)) {
            return true;
        }
        claim_one_slot();
        notify();
        return false;
    };

    if (!done()) {
        wake_cond.wait(lock, done);
    }
    return claim_or_shutdown();
}

bool
RateLimiter::fastpath_wait_for_opening()
{
    if (active_threads.load(std::memory_order_relaxed)
        <= max_active_threads.load(std::memory_order_relaxed)) {
        return false;
    }

    std::printf("fastpath wait_for_opening\n");

  //  running_threads.fetch_sub(1, std::memory_order_relaxed);
    free_one_slot();
    return wait_for_opening();
}

void
RateLimiter::notify()
{
    if (active_threads.load(std::memory_order_relaxed)
        < max_active_threads.load(std::memory_order_relaxed)) {
        std::printf("RateLimiter::notify()\n");
        wake_cond.notify_one();
    } 

    //else if (max_active_threads.load(std::memory_order_relaxed) == 0)
   // {
   //     notify_join();
   // }
}

void
RateLimiter::free_one_slot()
{
    auto res = active_threads.fetch_sub(1, std::memory_order_relaxed);
    std::printf("free one slot (prev %lu)\n", res);
    notify();
}

void RateLimiter::stop_threads()
{
    shutdown();
    max_active_threads = 0;

    wake_cond.notify_all();
}

/*
void
RateLimiter::join_threads()
{
    shutdown();
    max_active_threads = 0;

    std::printf("join thread start\n");

    wake_cond.notify_all();

    auto done = [&]() -> bool {
        return active_threads.load(std::memory_order_relaxed) == 0;
    };

    std::unique_lock lock(mtx);

    std::printf("active = %lu\n", active_threads.load(std::memory_order_relaxed));

    if (!done()) {
        join_cond.wait(lock, done);
    }

    std::printf("join threads done\n");
} */

} // namespace scs
