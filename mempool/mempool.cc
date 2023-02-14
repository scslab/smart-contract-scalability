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

#include "mempool/mempool.h"

namespace scs {

std::optional<SignedTransaction>
Mempool::get_new_tx()
{
    uint64_t i = indices.fetch_add(1, std::memory_order_acquire);

    uint32_t consumed_idx = i & 0xFFFF'FFFF;
    uint32_t filled_idx = i >> 32;

    if (consumed_idx >= filled_idx) {
        return std::nullopt;
    }
    std::optional<SignedTransaction> out
        = std::move(ringbuffer[consumed_idx % MAX_MEMPOOL_SIZE]);

    return out;
}

uint32_t
Mempool::available_size()
{
    uint64_t i = indices.load(std::memory_order_relaxed);

    uint32_t consumed_idx = i & 0xFFFF'FFFF;
    uint32_t filled_idx = i >> 32;

    if (consumed_idx > filled_idx)
    {
        return 0;
    }
    return (filled_idx - consumed_idx);
}

uint32_t
Mempool::add_txs(std::vector<SignedTransaction>&& txs)
{
    std::lock_guard lock(mtx);

    uint64_t i = indices.load(std::memory_order_acquire);

    uint32_t consumed_idx = i & 0xFFFF'FFFF;
    uint32_t filled_idx = i >> 32;

    if (consumed_idx > filled_idx)
    {
        consumed_idx = filled_idx;
    }

    // consumed_idx only increases, so this only overestimates space
    uint32_t used_space = filled_idx - consumed_idx;

    // underestimate
    uint32_t write_now
        = std::min<uint32_t>(MAX_MEMPOOL_SIZE - used_space, txs.size());

    for (size_t cpy = 0; cpy < write_now; cpy++) {
        ringbuffer[(filled_idx + cpy) % MAX_MEMPOOL_SIZE] = std::move(txs[cpy]);
    }

    filled_idx = (filled_idx + write_now) % MAX_MEMPOOL_SIZE;
    consumed_idx = consumed_idx % MAX_MEMPOOL_SIZE;

    indices.store((static_cast<uint64_t>(filled_idx) << 32) + consumed_idx,
                      std::memory_order_release);

    return write_now;
}

} // namespace scs
