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

#include "xdr/storage.h"
#include "xdr/storage_delta.h"
#include "xdr/types.h"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <utils/non_movable.h>

#include "contract_db/types.h"

namespace scs {

class ContractDB;

class UncommittedContracts : public utils::NonMovableOrCopyable
{
    // map contract hash to contract
    std::map<Hash, verified_contract_ptr_t> new_contracts;

    // map address to contract hash
    std::map<Address, Hash> new_deployments;

    // temporary bad solution
    std::mutex mtx;

  public:
    // caller must ensure script_hash exists
    bool deploy_contract_to_address(Address const& addr,
                                    Hash const& script_hash);

    void undo_deploy_contract_to_address(Address const& addr);

    void add_new_contract(Hash const& h, verified_contract_ptr_t new_contract);

    void commit(ContractDB& contract_db);

    void clear();
};

} // namespace scs
