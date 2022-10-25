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
}

bool
AtomicSet::try_insert(const Hash& h)
{
    const uint16_t start_idx = shorthash(h.data(), h.size(), capacity);
    uint16_t idx = start_idx;

    uint32_t alloc = ThreadlocalContextStore::allocate_hash(Hash(h));

    do {
        while (true) {
            uint32_t local = array[idx].load(std::memory_order_relaxed);

            if (local == 0) {
                if (array[idx].compare_exchange_strong(
                        local, alloc, std::memory_order_relaxed)) {
                    return true;
                } else {
                    continue;
                }
            }

            if (ThreadlocalContextStore::get_hash(local) == h) {
                return false;
            } else {
                break;
            }
        }

        idx++;
        if (idx == capacity) {
            idx = 0;
        }

    } while (idx != start_idx);

    throw std::runtime_error("too many inserts");
}
} // namespace scs