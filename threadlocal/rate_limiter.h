#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace scs {

class RateLimiter
{
    // threads overall running, possibly not with slots
  //  std::atomic<uint32_t> running_threads = 0;

    // threads with slots
    std::atomic<uint32_t> active_threads = 0;
    std::atomic<uint32_t> max_active_threads = 0;

    std::atomic<bool> _shutdown = false;

    std::condition_variable wake_cond;
    std::mutex mtx;

 //   std::condition_variable join_cond;

    void claim_one_slot() {
     //   running_threads.fetch_add(1, std::memory_order_relaxed); 
        active_threads.fetch_add(1, std::memory_order_relaxed);
    }

    void shutdown()
    {
        _shutdown = true;
        wake_cond.notify_all();
    }

  //  void notify_join()
  //  {
  //      if (active_threads.load(std::memory_order_relaxed) == 0) {
  //          std::lock_guard lock(mtx);
  //          join_cond.notify_one();
  //      }
  //  }

  public:
    bool wait_for_opening();

    void notify();

    void free_one_slot();

//    void notify_unslotted_stopped_running() {
//        running_threads.fetch_sub(1, std::memory_order_relaxed);
//        notify_join();
//    }

    bool fastpath_wait_for_opening();

    void start_threads(uint64_t max_active)
    {
        std::printf("starting worker threads\n");
    	std::lock_guard lock(mtx);
        max_active_threads = max_active;
        _shutdown = false;
        wake_cond.notify_all();
    }

    void stop_threads();

  //  void join_threads();

//    ~RateLimiter() { join_threads(); }
};

} // namespace scs
