#pragma once

#include <atomic>
#include <cstdint>

#include <chrono>
#include <condition_variable>
#include <mutex>

#include "xdr/transaction.h"

#include <utils/non_movable.h>

#include <optional>

namespace scs {

class AssemblyLimits
{
    std::atomic<int64_t> max_txs;
    std::atomic<int64_t> overall_gas_limit;
    //std::atomic<int64_t> active_threads;

    const uint64_t gas_limit_per_tx = 1'000'000;

    void undo_tx_reservation(uint64_t gas)
    {
        max_txs.fetch_add(1, std::memory_order_relaxed);
        overall_gas_limit.fetch_add(gas, std::memory_order_relaxed);
    }

    std::mutex mtx;
    std::condition_variable cv;
    bool shutdown = false;

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

    void notify_done();

  /*  void notify_activate_thread()
    {
        active_threads++;
    }
    void notify_deactivate_thread()
    {
        int64_t remaining = active_threads.fetch_sub(1, std::memory_order_relaxed);
        if (remaining == 1) {
            // self is last remaining thread
        }
        notify_done();
    } */

    std::optional<Reservation> reserve_tx(SignedTransaction const& tx);

    void wait_for(std::chrono::milliseconds timeout);
};

} // namespace scs
