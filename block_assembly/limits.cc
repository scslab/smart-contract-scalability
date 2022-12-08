#include "block_assembly/limits.h"

namespace scs {

std::optional<AssemblyLimits::Reservation>
AssemblyLimits::reserve_tx(SignedTransaction const& tx)
{
    if (tx.tx.gas_limit > gas_limit_per_tx) {
        return std::nullopt;
    }

    int64_t res = max_txs.fetch_sub(1, std::memory_order_relaxed);
    if (res <= 0) {
        max_txs.fetch_add(1, std::memory_order_relaxed);
        notify_done();
        return std::nullopt;
    }

    // can cause spurious failures, but that's ok
    res = overall_gas_limit.fetch_sub(tx.tx.gas_limit,
                                      std::memory_order_relaxed);
    if (res <= 0) {
        overall_gas_limit.fetch_add(tx.tx.gas_limit, std::memory_order_relaxed);
        notify_done();
        return std::nullopt;
    }

    return std::make_optional<Reservation>(tx.tx.gas_limit, *this);
}

void
AssemblyLimits::notify_done()
{
    std::lock_guard lock(mtx);
    shutdown = true;
    cv.notify_one();
}

void
AssemblyLimits::wait_for(std::chrono::milliseconds timeout)
{
    std::unique_lock lock(mtx);

    auto done = [this]() { return shutdown; };

    if (done()) {
        return;
    }

    cv.wait_for(lock, timeout, done);
}

} // namespace scs
