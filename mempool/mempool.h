#pragma once

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
