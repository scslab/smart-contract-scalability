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

namespace scs {

class ContractDB;
class TransactionRewind;

class ContractCreateClosure : public utils::NonCopyable
{
    Hash h;
    metered_contract_ptr_t contract;
    std::shared_ptr<const Contract> unmetered_contract;
    ContractDB& contract_db;

    bool do_create = false;

  public:
    ContractCreateClosure(Hash const& h, 
                          metered_contract_ptr_t contract,
                          std::shared_ptr<const Contract> unmetered_contract,
                          ContractDB& contract_db);

    ContractCreateClosure(ContractCreateClosure&& other);

    void commit();

    ~ContractCreateClosure();
};

class ContractDeployClosure
{
    const Address& deploy_address;
    const Hash& contract_hash;
    ContractDB& contract_db;

  public:
    ContractDeployClosure(const Address& deploy_address,
                          const Hash& contract_hash,
                          ContractDB& contract_db);

    void commit();
};

class ContractDBProxy
{
    ContractDB& contract_db;

    std::map<Hash, std::pair<metered_contract_ptr_t, std::shared_ptr<const Contract>>> new_contracts;

    std::map<Address, Hash> new_deployments;

    bool check_contract_exists(const Hash& contract_hash) const;

    ContractDeployClosure __attribute__((warn_unused_result))
    push_deploy_contract(const Address& deploy_address,
                         const Hash& contract_hash) const;

    ContractCreateClosure __attribute__((warn_unused_result))
    push_create_contract(Hash const& h, 
      std::pair<metered_contract_ptr_t, std::shared_ptr<const Contract>> const& contract);

    bool is_committed = false;
    void assert_not_committed() const;

  public:
    ContractDBProxy(ContractDB& db)
        : contract_db(db)
        , new_contracts()
        , new_deployments()
    {}

    // sender_address -- address of contract that creates the deployment
    // returns address at which contract is deployed (nullopt if error)
    std::optional<Address>
    deploy_contract(const Address& sender_address,
                    const Hash& contract_hash,
                    uint64_t nonce);

    // Only used for initialization -- must ensure that deploy address
    // can never collide with another potential deployment (i.e.
    // ensure deploy_address can't be a collision with some potential
    // output of the normal deploy_contract)
    void
    deploy_contract_at_specific_address(const Address& deploy_address,
      const Hash& contract_hash);

    Hash create_contract(std::shared_ptr<const Contract> contract);

    void push_updates_to_db(TransactionRewind& rewind);

    RunnableScriptView
    get_script(const Address& address) const;
};

} // namespace scs
