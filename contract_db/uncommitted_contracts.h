#pragma once

/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "metering_ffi/metered_contract.h"

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
    using metered_contract_ptr_t = std::shared_ptr<const MeteredContract>;

    // map contract hash to contract
    std::map<wasm_api::Hash, metered_contract_ptr_t> new_contracts;

    // map address to contract hash
    std::map<wasm_api::Hash, wasm_api::Hash> new_deployments;

    // temporary bad solution
    std::mutex mtx;

  public:
    // caller must ensure script_hash exists
    bool deploy_contract_to_address(wasm_api::Hash const& addr,
                                    wasm_api::Hash const& script_hash);

    void undo_deploy_contract_to_address(wasm_api::Hash const& addr);

    void add_new_contract(wasm_api::Hash const& h, std::shared_ptr<const MeteredContract> new_contract);

    void commit(ContractDB& contract_db);

    void clear();
};

} // namespace scs
