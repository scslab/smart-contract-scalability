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

#include "block_assembly/limits.h"

namespace scs {

std::optional<AssemblyLimits::Reservation>
AssemblyLimits::reserve_tx(SignedTransaction const& tx)
{
    if (tx.tx.gas_limit > gas_limit_per_tx) {
        return std::nullopt;
    }

    int64_t res = max_txs.fetch_sub(1, std::memory_order_relaxed);
    if (res <= 0) {
        max_txs.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }

    // can cause spurious failures, but that's ok
    res = overall_gas_limit.fetch_sub(tx.tx.gas_limit,
                                      std::memory_order_relaxed);
    if (res <= 0) {
        overall_gas_limit.fetch_add(tx.tx.gas_limit, std::memory_order_relaxed);
        return std::nullopt;
    }

    return std::make_optional<Reservation>(tx.tx.gas_limit, *this);
}

void
AssemblyLimits::notify_done()
{
    std::lock_guard lock(mtx);
    shutdown = true;
    cv.notify_one();
}

void
AssemblyLimits::wait_for(std::chrono::milliseconds timeout)
{
    std::unique_lock lock(mtx);

    auto done = [this]() { return shutdown; };

    if (done()) {
        return;
    }

    cv.wait_for(lock, timeout, done);
}

} // namespace scs
