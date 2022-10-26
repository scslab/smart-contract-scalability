#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

#include "xdr/types.h"

namespace scs {

class AtomicSet
{
    constexpr static float extra_buffer = 1.2;

    uint16_t capacity = 0;

    std::atomic<uint32_t>* array = nullptr;

    std::atomic<uint16_t> num_filled_slots = 0;

    constexpr static uint32_t TOMBSTONE = 0xFFFF'FFFF;

  public:
    AtomicSet(uint16_t max_capacity)
        : capacity(max_capacity * extra_buffer)
        , array(nullptr)
    {
        if (capacity > 0)
        {
           array = new std::atomic<uint32_t>[capacity] {};
        }
    }

    ~AtomicSet();

    void resize(uint16_t new_capacity);

    void clear();

    bool try_insert(const Hash& h);

    // throws if nexist
    void erase(const Hash& h);

    std::vector<Hash> get_hashes() const;
};

} // namespace scs
