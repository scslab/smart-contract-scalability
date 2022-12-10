#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

#include "xdr/types.h"
#include "xdr/storage.h"

#include <utils/non_movable.h>

namespace scs {

class AtomicSet : public utils::NonMovableOrCopyable
{
    constexpr static float extra_buffer = 1.2;

    uint32_t capacity = 0;

    std::atomic<uint32_t>* array = nullptr;

    std::atomic<uint32_t> num_filled_slots = 0;

    constexpr static uint32_t TOMBSTONE = 0xFFFF'FFFF;

  public:
    AtomicSet(uint32_t max_capacity)
        : capacity(max_capacity * extra_buffer)
        , array(nullptr)
    {
        if (capacity > 0)
        {
           array = new std::atomic<uint32_t>[capacity] {};
        }
    }

    ~AtomicSet();

    void resize(uint32_t new_capacity);

    void clear();

    bool try_insert(const HashSetEntry& h);

    // throws if nexist
    void erase(const HashSetEntry& h);

    std::vector<HashSetEntry> get_hashes() const;
};

} // namespace scs
