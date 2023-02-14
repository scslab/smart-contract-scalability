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

#include "threadlocal/rate_limiter.h"

namespace scs {

RateLimiter::RateLimiter()
{
    bool prev = false;
    if (!unique_init.compare_exchange_strong(prev, true)) {
        throw std::runtime_error("double init of rate_limiter");
    }
};

bool
RateLimiter::slowpath_wait_for_opening()
{
    auto done = [&]() -> bool {
        bool res = _shutdown.load(std::memory_order_relaxed)
                   || (active_threads < max_active_threads);
        return res;
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

    free_one_slot();
    return slowpath_wait_for_opening();
}

bool
RateLimiter::wait_for_opening()
{
    if (has_slot) {
        return fastpath_wait_for_opening();
    } else {
        return slowpath_wait_for_opening();
    }
}

void
RateLimiter::start_threads(uint64_t max_active)
{
    std::lock_guard lock(mtx);
    max_active_threads = max_active;
    if (_shutdown) {
        throw std::runtime_error("potential race condition between starting "
                                 "threads and setting shutdown to false!");
    }
    wake_cond.notify_all();
}

void
RateLimiter::prep_for_notify()
{
    std::lock_guard lock(mtx);
    _shutdown = false;
}

void
RateLimiter::notify()
{
    if (active_threads.load(std::memory_order_relaxed)
        < max_active_threads.load(std::memory_order_relaxed)) {
        wake_cond.notify_one();
    }
}

void
RateLimiter::free_one_slot()
{
    if (has_slot) {
        auto res = active_threads.fetch_sub(1, std::memory_order_relaxed);
    }
    has_slot = false;
    notify();
}

void
RateLimiter::claim_one_slot()
{
    if (has_slot) {
        throw std::runtime_error("double claim slot");
    }
    has_slot = true;
    active_threads.fetch_add(1, std::memory_order_relaxed);
}

void
RateLimiter::stop_threads()
{
    shutdown();
    max_active_threads = 0;

    wake_cond.notify_all();
}

} // namespace scs
