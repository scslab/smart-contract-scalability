#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace scs {

class RateLimiter
{
    std::atomic<uint32_t> active_threads;
    std::atomic<uint32_t> max_active_threads;

    std::atomic<bool> _shutdown = false;

    std::condition_variable wake_cond;
    std::mutex mtx;

    std::condition_variable join_cond;

    void claim_one_slot() { active_threads++; }

    void shutdown()
    {
        _shutdown = true;
        wake_cond.notify_all();
    }

    void notify_join()
    {
        if (active_threads.load(std::memory_order_relaxed) == 0) {
            join_cond.notify_one();
        }
    }

  public:
    bool wait_for_opening()
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

    void notify()
    {
        if (active_threads.load(std::memory_order_relaxed)
            < max_active_threads.load(std::memory_order_relaxed)) {
            wake_cond.notify_one();
        } else // always if max_active_threads == 0, not usually in normal case
               // operation
        {
            notify_join();
        }
    }

    void free_one_slot()
    {
        active_threads.fetch_sub(1, std::memory_order_relaxed);
        notify();
    }

    bool fastpath_wait_for_opening()
    {
        if (active_threads.load(std::memory_order_relaxed)
            <= max_active_threads.load(std::memory_order_relaxed)) {
            return false;
        }

        free_one_slot();
        return wait_for_opening();
    }

    void start_threads(uint64_t max_active)
    {
        max_active_threads = max_active;
        wake_cond.notify_all();
    }

    void join_threads();

    ~RateLimiter() { join_threads(); }
};

} // namespace scs
