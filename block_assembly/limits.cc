#include "block_assembly/limits.h"

namespace scs {

std::optional<Reservation>
AssemblyLimits::reserve_tx(SignedTransaction const& tx)
{
    if (tx.tx.gas_limit > gas_limit_per_tx) {
        return std::nullopt;
    }

    int64_t res = max_txs.fetch_sub(1, std::memory_order_relaxed);
    if (res <= 0) {
        max_txs.fetch_add(1, std::memory_order_relaxed);
        return std::nullopt;
    }

    // can cause spurious failures, but that's ok
    res = overall_gas_limit.fetch_sub(tx.tx.gas_limit,
                                      std::memory_order_relaxed);
    if (res <= 0) {
        overall_gas_limit.fetch_add(tx.tx.gas_limit, std::memory_order_relaxed);
        return std::nullopt;
    }

    return std::make_optional<Reservation>(tx.tx.gas_limit, *this);
}

} // namespace scs
