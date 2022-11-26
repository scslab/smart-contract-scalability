#pragma once

namespace scs {

class RateLimiter
{
    std::atomic<uint32_t> active_threads;
    std::atomic<uint32_t> max_active_threads;

    std::atomic<bool> _shutdown = false;

    std::condition_variable wake_cond;
    std::mutex mtx;

    void claim_one_slot() { active_threads++; }

  public:
    void wait_for_opening()
    {
        auto done = [&]() {
            return _shutdown.load(std::memory_order_relaxed)
                   || (active_threads < max_active_threads);
        };

        std::unique_lock lock(mtx);
        if (done()) {
            claim_one_slot();
            notify();
            return;
        }
        wake_cond.wait(lock, done);
        notify();
    }

    void notify()
    {
        if (active_threads < max_active_threads) {
            wake_cond.notify_one();
        }
    }

    void free_one_slot()
    {
        active_threads--;
        notify();
    }

    void shutdown()
    {
        _shutdown = true;
        wake_cond.notify_all();
    }

    bool is_shutdown() const { return _shutdown; }
};

} // namespace scs