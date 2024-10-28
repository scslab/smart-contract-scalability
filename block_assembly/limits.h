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
#include <cstdint>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>

#include "xdr/transaction.h"

#include <utils/non_movable.h>

namespace scs {

class AssemblyLimits
{
    std::atomic<int64_t> max_txs;
    std::atomic<int64_t> overall_gas_limit;

    const uint64_t gas_limit_per_tx = 100'000'000'000;

    void undo_tx_reservation(uint64_t gas)
    {
        max_txs.fetch_add(1, std::memory_order_relaxed);
        overall_gas_limit.fetch_add(gas, std::memory_order_relaxed);
    }

    std::mutex mtx;
    std::condition_variable cv;
    bool shutdown = false;

  public:
    AssemblyLimits(int64_t max_txs, int64_t overall_gas_limit)
        : max_txs(max_txs)
        , overall_gas_limit(overall_gas_limit)
    {}

    class Reservation : public utils::NonMovableOrCopyable
    {
        uint64_t gas;
        AssemblyLimits& main;

      public:
        Reservation(uint64_t gas, AssemblyLimits& main)
            : gas(gas)
            , main(main)
        {}

        void commit() { gas = 0; }

        ~Reservation()
        {
            if (gas > 0) {
                main.undo_tx_reservation(gas);
            }
        }
    };

    void notify_done();

    std::optional<Reservation> reserve_tx(SignedTransaction const& tx);

    void wait_for(std::chrono::milliseconds timeout);
};

} // namespace scs
