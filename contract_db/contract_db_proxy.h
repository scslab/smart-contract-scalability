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

#include <utils/non_movable.h>

#include "metering_ffi/metered_contract.h"

#include "xdr/types.h"

#include <map>
#include <memory>
#include <optional>

#include <wasm_api/wasm_api.h>

namespace scs {

class ContractDB;
class TransactionRewind;

class ContractCreateClosure : public utils::NonCopyable
{
    wasm_api::Hash h;
    std::shared_ptr<const MeteredContract> contract;
    ContractDB& contract_db;

    bool do_create = false;

  public:
    ContractCreateClosure(wasm_api::Hash h, 
                          std::shared_ptr<const MeteredContract> contract,
                          ContractDB& contract_db);

    ContractCreateClosure(ContractCreateClosure&& other);

    void commit();

    ~ContractCreateClosure();
};

class ContractDeployClosure : public utils::NonCopyable
{
    const wasm_api::Hash& deploy_address;
    ContractDB& contract_db;

    bool undo_deploy = true;

  public:
    ContractDeployClosure(const wasm_api::Hash& deploy_address,
                          ContractDB& contract_db);

    ContractDeployClosure(ContractDeployClosure&& other);

    void commit();

    ~ContractDeployClosure();
};

class ContractDBProxy
{
    ContractDB& contract_db;

    std::map<wasm_api::Hash, std::shared_ptr<const MeteredContract>> new_contracts;

    std::map<wasm_api::Hash, wasm_api::Hash> new_deployments;

    bool check_contract_exists(const Hash& contract_hash) const;

    std::optional<ContractDeployClosure> __attribute__((warn_unused_result))
    push_deploy_contract(const wasm_api::Hash& deploy_address,
                         const wasm_api::Hash& contract_hash);

    ContractCreateClosure __attribute__((warn_unused_result))
    push_create_contract(wasm_api::Hash const& h, std::shared_ptr<const MeteredContract> contract);

    bool is_committed = false;
    void assert_not_committed() const;

  public:
    ContractDBProxy(ContractDB& db)
        : contract_db(db)
        , new_contracts()
        , new_deployments()
    {}

    bool __attribute__((warn_unused_result))
    deploy_contract(const Address& deploy_address,
                         const Hash& contract_hash);

    Hash create_contract(std::shared_ptr<const Contract> contract);

    bool push_updates_to_db(TransactionRewind& rewind);

    wasm_api::Script
    get_script(const wasm_api::Hash& address) const;
};

} // namespace scs
