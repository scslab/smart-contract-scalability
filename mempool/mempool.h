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
#include <mutex>
#include <optional>

#include "xdr/transaction.h"

namespace scs {

class Mempool
{
    constexpr static uint64_t MAX_MEMPOOL_SIZE = static_cast<uint32_t>(1) << 24;

    std::vector<SignedTransaction> ringbuffer;
    std::atomic<uint64_t> indices;

    std::mutex mtx; // for add_txs

  public:
    Mempool()
        : ringbuffer()
        , indices(0)
    {
        ringbuffer.resize(MAX_MEMPOOL_SIZE);
    }

    std::optional<SignedTransaction> get_new_tx();
    uint32_t available_size();

    uint32_t add_txs(std::vector<SignedTransaction>&& txs);
};

} // namespace scs
