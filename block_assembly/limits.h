#pragma once

#include <atomic>
#include <cstdint>

#include "xdr/transaction.h"

#include <utils/non_movable.h>

namespace scs {

class AssemblyLimits
{
    std::atomic<int64_t> max_txs;
    std::atomic<int64_t> overall_gas_limit;
    const uint64_t gas_limit_per_tx = 1'000'000;

    void undo_tx_reservation(uint64_t gas)
    {
        max_txs.fetch_add(1, std::memory_order_relaxed);
        overall_gas_limit.fetch_add(gas, std::memory_order_relaxed);
    }

  public:
    AssemblyLimits(int64_t max_txs, int64_t overall_gas_limit)
        : max_txs(max_txs)
        , overall_gas_limit(overall_gas_limit)
    {}

    class Reservation : public utils::NonMovableOrCopyable
    {
        uint64_t gas;
        AssemblyLimits& main;

      public:
        Reservation(uint64_t gas, AssemblyLimits& main)
            : gas(gas)
            , main(main)
        {}

        void commit() { gas = 0; }

        ~Reservation()
        {
            if (gas > 0) {
                main.undo_tx_reservation(gas);
            }
        }
    };

    std::optional<Reservation> reserve_tx(SignedTransaction const& tx);
};

} // namespace scs
