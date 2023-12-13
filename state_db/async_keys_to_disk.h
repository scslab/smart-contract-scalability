#pragma once

#include <utils/async_worker.h>

#include <mutex>
#include <condition_variable>

#include "utils/save_load_xdr.h"

#include <utils/mkdir.h>

#include "config/static_constants.h"

#include "persistence/rocksdb_iface.h"

namespace scs {

class AsyncKeysToDisk : public utils::AsyncWorker
{

    const std::string folder = "sisyphusdb_logs/";

    std::string 
    get_filename(uint32_t ts) {
        return folder + std::to_string(ts);
    }

    void make_folder()
    {
        utils::mkdir_safe(folder);
    }

    std::array<std::vector<uint8_t>, TLCACHE_SIZE> work_item;
    bool work_done = true;
    uint32_t work_ts = 0;

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

            std::string filename = get_filename(work_ts);

            
           	FILE* f = std::fopen(filename.c_str(), "w");

			if (f == nullptr) {
				throw std::runtime_error("failed to open file");
			}

			for (auto& buf : work_item)
			{
				std::fwrite(buf.data(), sizeof(buf.data()[0]), buf.size(), f);
				buf.clear();
			}

			std::fflush(f);
			fsync(fileno(f));
			std::fclose(f);

		    work_done = true;
            cv.notify_all();
        }
    }

  public:
    AsyncKeysToDisk()
        : utils::AsyncWorker()
        , work_item()
    {
        make_folder();
        start_async_thread([this] { run(); });
    }

    ~AsyncKeysToDisk() { terminate_worker(); }

    void log_keys(trie::SerializeDiskInterface<sizeof(AddressAndKey), TLCACHE_SIZE>& storage_iface, uint32_t timestamp)
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

    void clear_folder()
    {
        utils::clear_directory(folder);
    }

    using AsyncWorker::wait_for_async_task;
};

} // namespace scs
