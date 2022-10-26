#include "hash_set/atomic_set.h"

#include "crypto/crypto_utils.h"

#include "threadlocal/threadlocal_context.h"

namespace scs {

void
AtomicSet::resize(uint16_t new_capacity)
{
    uint16_t new_alloc_size = new_capacity * extra_buffer;
    if (new_alloc_size < capacity) {
        return;
    }

    delete[] array;
    capacity = new_alloc_size;
    array = new std::atomic<uint32_t>[capacity] {};

    num_filled_slots = 0;
}

void
AtomicSet::clear()
{
    for (auto i = 0u; i < capacity; i++) {
        array[i] = 0;
    }
    num_filled_slots = 0;
}

bool
AtomicSet::try_insert(const Hash& h)
{
    const uint16_t start_idx = shorthash(h.data(), h.size(), capacity);
    uint16_t idx = start_idx;

    uint32_t alloc = ThreadlocalContextStore::allocate_hash(Hash(h));

    const uint16_t cur_filled_slots
        = num_filled_slots.load(std::memory_order_relaxed);

    if (cur_filled_slots >= capacity) {
        return false;
    }

    do {
        while (true) {
            uint32_t local = array[idx].load(std::memory_order_relaxed);

            if (local == 0) {
                if (array[idx].compare_exchange_strong(
                        local, alloc, std::memory_order_relaxed)) {

                    num_filled_slots.fetch_add(1, std::memory_order_relaxed);

                    return true;
                } else {
                    continue;
                }
            }

            if (local != TOMBSTONE) {
                if (ThreadlocalContextStore::get_hash(local) == h) {
                    return false;
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        idx++;
        if (idx == capacity) {
            idx = 0;
        }

    } while (idx != start_idx);

    return false;
}

void
AtomicSet::erase(const Hash& h)
{
    const uint16_t start_idx = shorthash(h.data(), h.size(), capacity);
    uint16_t idx = start_idx;

    do {
        while (true) {
            uint32_t local = array[idx].load(std::memory_order_relaxed);

            if (local != 0 && local != TOMBSTONE) {
                if (ThreadlocalContextStore::get_hash(local) == h) {

                    if (array[idx].compare_exchange_strong(
                            local, TOMBSTONE, std::memory_order_relaxed)) {
                        return;
                    } else {
                        continue;
                    }
                }
            }

            if (local == 0) {
                throw std::runtime_error("deletion failed to find elt");
            }
            break;
        }

        idx++;
        if (idx == capacity) {
            idx = 0;
        }

    } while (idx != start_idx);

    throw std::runtime_error("deletion failed after complete scan");
}

} // namespace scs