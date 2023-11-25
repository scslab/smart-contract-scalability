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

#include <utils/threadlocal_cache.h>

namespace scs {

class AsyncPersistContracts : public utils::AsyncWorker
{
    const std::string folder;

    struct ContractRoundPersistence
    {
        struct ContractCreate
        {
            wasm_api::Hash const h;
            std::shared_ptr<const Contract> unmetered_contract;
        };

        struct ContractDeploy
        {
            wasm_api::Hash const contract_addr;
            wasm_api::Hash const contract_hash;
        };

        std::vector<ContractCreate> creations;
        std::vector<ContractDeploy> deployments;
    };

    std::string get_contract_filename(wasm_api::Hash const& h)
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

    void swap_data()
    {
        auto& objs = cache.get_objects();
        for (size_t i = 0; i < work_item.size(); i++) {
            if (!objs[i]) {
                objs[i].emplace();
            }
            work_item[i].creations.clear();
            std::swap(work_item[i].creations, objs[i]->creations);
            work_item[i].deployments.clear();
            std::swap(work_item[i].deployments, objs[i]->deployments);
        }
    }

    void save_contract(ContractRoundPersistence::ContractCreate const& create)
    {
        auto filename = get_contract_filename(create.h);

        FILE* f = std::fopen(filename.c_str(), "w");

        if (f == nullptr) {
            throw std::runtime_error("failed to open deployfile");
        }

        std::fwrite(create.unmetered_contract->data(),
                    sizeof(uint8_t),
                    create.unmetered_contract->size(),
                    f);
        std::fflush(f);
        fsync(fileno(f));
        std::fclose(f);
    }

    void save_data()
    {
        auto filename = get_deploy_filename(work_ts);

        FILE* f = std::fopen(filename.c_str(), "w");

        if (f == nullptr) {
            throw std::runtime_error("failed to open deployfile");
        }

        for (auto const& obj : work_item) {
            for (auto const& deploy : obj.deployments) {
                std::fwrite(deploy.contract_addr.data(),
                            sizeof(uint8_t),
                            deploy.contract_addr.size(),
                            f);
                std::fwrite(deploy.contract_hash.data(),
                            sizeof(uint8_t),
                            deploy.contract_hash.size(),
                            f);
            }
            for (auto const& create : obj.creations) {
                save_contract(create);
            }
        }

        std::fflush(f);

        fsync(fileno(f));

        std::fclose(f);
    }

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

            save_data();

            swap_data();
            work_done = true;
            cv.notify_all();
        }
    }

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

    void log_deploy(wasm_api::Hash const& contract_addr,
                    const wasm_api::Hash& contract_hash)
    {
        if constexpr (PERSISTENT_STORAGE_ENABLED) {
            cache.get().deployments.emplace_back(contract_addr, contract_hash);
        }
    }

    void log_create(const wasm_api::Hash& hash,
                    std::shared_ptr<const Contract> contract)
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
