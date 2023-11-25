#pragma once

#include "xdr/storage_delta.h"

#include <utils/async_worker.h>

#include <mutex>
#include <condition_variable>

#include "utils/save_load_xdr.h"

#include <utils/mkdir.h>

#include "config/static_constants.h"

namespace scs {

template<typename xdr_type>
class AsyncPersistXDR : public utils::AsyncWorker
{
    const std::string folder;

    std::string 
    get_filename(uint32_t ts) {
        return folder + std::to_string(ts);
    }

    void make_folder()
    {
        utils::mkdir_safe(folder);
    }

    xdr_type work_item;
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

            if (save_xdr_to_file(work_item, filename.c_str()) != 0)
            {
                throw std::runtime_error("file save failed");
            }

            work_done = true;
            cv.notify_all();
        }
    }

  public:
    AsyncPersistXDR(std::string folder)
        : utils::AsyncWorker()
        , folder(folder)
        , work_item()
    {
        if constexpr (PERSISTENT_STORAGE_ENABLED)
        {
            make_folder();
            start_async_thread([this] { run(); });
        }
    }

    ~AsyncPersistXDR()
    { 
        if constexpr (PERSISTENT_STORAGE_ENABLED)
        {
            terminate_worker(); 
        }
    }

    void log(xdr_type& block, uint32_t timestamp)
    {
        if constexpr (!PERSISTENT_STORAGE_ENABLED)
        {
            return;
        }
        std::lock_guard lock(mtx);
        std::swap(block, work_item);
        work_done = false;
        work_ts = timestamp;
        cv.notify_all();
    }

    void clear_folder()
    {
        if constexpr (PERSISTENT_STORAGE_ENABLED)
        {
            utils::clear_directory(folder);
            make_folder();
        }
    }

    using AsyncWorker::wait_for_async_task;
};

} // namespace scs
