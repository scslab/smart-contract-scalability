#pragma once

#include "xdr/storage.h"
#include "xdr/storage_delta.h"
#include "xdr/types.h"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <wasm_api/wasm_api.h>

#include <utils/non_movable.h>

namespace scs {

class ContractDB;

class UncommittedContracts : public utils::NonMovableOrCopyable
{
    // map contract hash to contract
    std::map<wasm_api::Hash, std::shared_ptr<const Contract>> new_contracts;

    // map address to contract hash
    std::map<wasm_api::Hash, wasm_api::Hash> new_deployments;

    // temporary bad solution
    std::mutex mtx;

    void clear();

  public:
    // caller must ensure script_hash exists
    bool deploy_contract_to_address(wasm_api::Hash const& addr,
                                    wasm_api::Hash const& script_hash);

    void undo_deploy_contract_to_address(wasm_api::Hash const& addr);

    void add_new_contract(std::shared_ptr<const Contract> new_contract);

    void commit(ContractDB& contract_db);
};

} // namespace scs
