#include "contract_db/contract_persistence.h"

namespace scs {

void
AsyncPersistContracts::swap_data()
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

void
AsyncPersistContracts::save_contract(
    ContractRoundPersistence::ContractCreate const& create)
{
    auto filename = get_contract_filename(create.h);

    FILE* f = std::fopen(filename.c_str(), "w");

    if (f == nullptr) {
        throw std::runtime_error("failed to open deployfile");
    }

    std::fwrite(create.contract->bytes.data(),
                sizeof(uint8_t),
                create.contract->bytes.size(),
                f);
    std::fflush(f);
    fsync(fileno(f));
    std::fclose(f);
}

void
AsyncPersistContracts::save_data()
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

void
AsyncPersistContracts::run()
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

} // namespace scs
