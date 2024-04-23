#pragma once

/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

    inline static thread_local bool has_slot = false;

    //   std::condition_variable join_cond;

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

    bool fastpath_wait_for_opening();
    bool slowpath_wait_for_opening();

    inline static std::atomic<bool> unique_init = false;

  public:
    RateLimiter();

    bool wait_for_opening();

    void notify();

    void claim_one_slot();
    void free_one_slot();

    void start_threads(uint64_t max_active);

    void prep_for_notify();

    void stop_threads();

    //  void join_threads();

    //    ~RateLimiter() { join_threads(); }
};

} // namespace scs
