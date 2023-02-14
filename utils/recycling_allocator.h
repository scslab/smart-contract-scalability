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

#include <utils/non_movable.h>

namespace scs
{

template<typename T, uint32_t BUFFER_SIZE>
class RecyclingAllocatorLine : public utils::NonMovableOrCopyable
{
    void* buffer;

    constexpr static size_t BUF_SIZE_BYTES = sizeof(T) * BUFFER_SIZE;

    std::atomic<uint32_t> active_size;

    uint32_t prev_active_size;

public:

    RecyclingAllocatorLine()
        : buffer(malloc(BUF_SIZE_BYTES))
        , active_size(0)
        , prev_active_size(0)
    {}

    T* 
    allocate()
    {
        uint32_t slot = active_size.fetch_add(1, std::memory_order_relaxed);

        if (slot >= BUFFER_SIZE)
        {
            return nullptr;
        }

        T* ptr = (static_cast<T*>(buffer) + slot);

        if (slot < prev_active_size)
        {
            ptr -> ~T();
        }

        return new (static_cast<void*>(ptr)) T();
    }

    void clear_and_reset()
    {
        prev_active_size = std::max(prev_active_size, active_size.load(std::memory_order_relaxed));
        active_size = 0;
    }

    ~RecyclingAllocatorLine()
    {
        clear_and_reset();

        for (uint32_t i = 0; i < std::min(prev_active_size, BUFFER_SIZE); i++)
        {
            T* ptr = (static_cast<T*>(buffer) + i);
            ptr -> ~T();
        }

        free(buffer);
    }
};




}