#pragma once

#include "xdr/storage_delta.h"

#include <utils/async_worker.h>

#include <condition_variable>
#include <mutex>

#include "utils/save_load_xdr.h"

#include "debug/debug_utils.h"

#include <utils/mkdir.h>

#include <utils/threadlocal_cache.h>

#include <fcntl.h>

#include "config/static_constants.h"

#include "contract_db/verified_script.h"

#include <utils/threadlocal_cache.h>

namespace scs {

class AsyncPersistContracts : public utils::AsyncWorker
{
    const std::string folder;

    struct ContractRoundPersistence
    {
        struct ContractCreate
        {
            Hash const h;
            verified_script_ptr_t contract;
        };

        struct ContractDeploy
        {
            Address const contract_addr;
            Hash const contract_hash;
        };

        std::vector<ContractCreate> creations;
        std::vector<ContractDeploy> deployments;
    };

    std::string get_contract_filename(Hash const& h)
    {
        return folder + "contracts/" + debug::array_to_str(h);
    }

    std::string get_deploy_filename(uint32_t round)
    {
        return folder + std::to_string(round);
    }

    void make_folder()
    {
        utils::mkdir_safe(folder);
        utils::mkdir_safe(folder + "contracts/");
    }

    std::array<ContractRoundPersistence, TLCACHE_SIZE> work_item;
    utils::ThreadlocalCache<ContractRoundPersistence, TLCACHE_SIZE> cache;

    bool work_done = true;
    uint32_t work_ts = 0;

    bool exists_work_to_do() override final { return !work_done; }

    void swap_data();

    void save_contract(ContractRoundPersistence::ContractCreate const& create);

    void save_data();

    void run();

  public:
    AsyncPersistContracts(std::string folder)
        : utils::AsyncWorker()
        , folder(folder)
        , work_item()
    {
        if constexpr (PERSISTENT_STORAGE_ENABLED) {
            make_folder();
            start_async_thread([this] { run(); });
        }
    }

    ~AsyncPersistContracts()
    {
        if constexpr (PERSISTENT_STORAGE_ENABLED) {
            terminate_worker();
        }
    }

    void log_deploy(Address const& contract_addr,
                    const Hash& contract_hash)
    {
        if constexpr (PERSISTENT_STORAGE_ENABLED) {
            cache.get().deployments.emplace_back(contract_addr, contract_hash);
        }
    }

    void log_create(const Hash& hash,
                    verified_script_ptr_t contract)
    {
        if constexpr (PERSISTENT_STORAGE_ENABLED) {
            cache.get().creations.emplace_back(hash, contract);
        }
    }
    void write(uint32_t timestamp)
    {
        if constexpr (PERSISTENT_STORAGE_ENABLED) {
            std::lock_guard lock(mtx);
            work_done = false;
            work_ts = timestamp;
            cv.notify_all();
        }
    }
    void nowrite()
    {
        if constexpr (PERSISTENT_STORAGE_ENABLED) {
            std::lock_guard lock(mtx);
            work_done = true;
            swap_data();
            cv.notify_all();
        }
    }

    void clear_folder()
    {
        if constexpr (PERSISTENT_STORAGE_ENABLED) {
            utils::clear_directory(folder);
            make_folder();
        }
    }

    using AsyncWorker::wait_for_async_task;
};

} // namespace scs
