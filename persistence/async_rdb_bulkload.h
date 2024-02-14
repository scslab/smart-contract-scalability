#pragma once

#include <utils/async_worker.h>

#include <mutex>
#include <condition_variable>

#include "utils/save_load_xdr.h"

#include <utils/mkdir.h>

#include "config/static_constants.h"

#include "persistence/accumulate_kvs_iface.h"

#include "persistence/rocksdb_wrapper.h"

namespace scs {

class AsyncRDBBulkLoad : public utils::AsyncWorker
{

    using pair_t = std::pair<trie::TimestampPointerPair, trie::DurableValue<sizeof(AddressAndKey)>>;

    std::array<std::vector<pair_t>, TLCACHE_SIZE> work_item;
    bool work_done = true;
    uint32_t work_ts = 0;

    RocksdbWrapper& rdb;

    bool exists_work_to_do() override final { return !work_done; }

    void run()
    {
        while (true) {
            std::unique_lock lock(mtx);

            if ((!done_flag) && (!exists_work_to_do())) {
                cv.wait(lock,
                        [this]() { return done_flag || exists_work_to_do(); });
            }
            if (done_flag) {
                return;
            }

            throw std::runtime_error("unimpl");

            //TODO

		    work_done = true;
            cv.notify_all();
        }
    }

  public:
    AsyncRDBBulkLoad(RocksdbWrapper& rdb)
        : utils::AsyncWorker()
        , work_item()
        , rdb(rdb)
    {
        start_async_thread([this] { run(); });
    }

    ~AsyncRDBBulkLoad() { terminate_worker(); }

    void log_keys(AccumulateKVsInterface<sizeof(AddressAndKey), TLCACHE_SIZE>& storage_iface, uint32_t timestamp)
    {
        std::lock_guard lock(mtx);
        storage_iface.swap_buffers(work_item);
        work_done = false;
        work_ts = timestamp;
        cv.notify_all();
    }

    void log_keys(DirectWriteRocksDBIface<sizeof(AddressAndKey)> const& rdb, uint32_t timestamp)
    {}

    void log_keys(trie::NullInterface<sizeof(AddressAndKey)>& iface, uint32_t timestamp)
    {}

    using AsyncWorker::wait_for_async_task;
};

} // namespace scs
