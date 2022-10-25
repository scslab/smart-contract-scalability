#pragma once

#include <atomic>
#include <cstdint>

#include "xdr/types.h"

namespace scs {

class AtomicSet
{
    constexpr static float extra_buffer = 1.2;

    uint16_t capacity = 0;

    std::atomic<uint32_t>* array = nullptr;

  public:
    AtomicSet(uint16_t max_capacity)
        : capacity(max_capacity * extra_buffer)
        , array(new std::atomic<uint32_t>[capacity] {})
    {}

    void resize(uint16_t new_capacity);

    bool try_insert(const Hash& h);
};

} // namespace scs
