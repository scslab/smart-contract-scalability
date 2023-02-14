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

#include "contract_db/contract_db_proxy.h"

#include "contract_db/contract_db.h"

#include "crypto/hash.h"

#include "storage_proxy/transaction_rewind.h"

#include "debug/debug_utils.h"

namespace scs {

ContractCreateClosure::ContractCreateClosure(
    wasm_api::Hash h, 
    std::shared_ptr<const MeteredContract> contract,
    ContractDB& contract_db)
    : h(h)
    , contract(contract)
    , contract_db(contract_db)
{}

ContractCreateClosure::ContractCreateClosure(ContractCreateClosure&& other)
    : h(other.h)
    , contract(other.contract)
    , contract_db(other.contract_db)
    , do_create(other.do_create)
{
    other.do_create = false;
}

void
ContractCreateClosure::commit()
{
    do_create = true;
}

ContractCreateClosure::~ContractCreateClosure()
{
    if (do_create) {
        contract_db.add_new_uncommitted_contract(h, std::move(contract));
    }
}

ContractDeployClosure::ContractDeployClosure(
    const wasm_api::Hash& deploy_address,
    ContractDB& contract_db)
    : deploy_address(deploy_address)
    , contract_db(contract_db)
{}

ContractDeployClosure::ContractDeployClosure(ContractDeployClosure&& other)
    : deploy_address(other.deploy_address)
    , contract_db(other.contract_db)
    , undo_deploy(other.undo_deploy)
{
    other.undo_deploy = false;
}

void
ContractDeployClosure::commit()
{
    undo_deploy = false;
}

ContractDeployClosure::~ContractDeployClosure()
{
    if (undo_deploy) {
        contract_db.undo_deploy_contract_to_address(deploy_address);
    }
}

bool
ContractDBProxy::check_contract_exists(const Hash& contract_hash) const
{
    if (contract_db.check_committed_contract_exists(contract_hash)) {
        return true;
    }
    return new_contracts.find(contract_hash) != new_contracts.end();
}

bool
ContractDBProxy::deploy_contract(const Address& deploy_address,
                                 const Hash& contract_hash)
{
/*
    std::printf("attempting to deploy contract hash %s to address %s\n",
        debug::array_to_str(contract_hash).c_str(),
        debug::array_to_str(deploy_address).c_str());
*/
    if (!check_contract_exists(contract_hash)) {
        return false;
    }
    if (new_deployments.find(deploy_address) != new_deployments.end()) {
        return false;
    }

    if (!contract_db.check_address_open_for_deployment(deploy_address)) {
        return false;
    }
    new_deployments[deploy_address] = contract_hash;
    return true;
}

std::optional<ContractDeployClosure>
ContractDBProxy::push_deploy_contract(const wasm_api::Hash& deploy_address,
                                      const wasm_api::Hash& contract_hash)
{
    if (!contract_db.deploy_contract_to_address(deploy_address,
                                                contract_hash)) {
        return std::nullopt;
    }

    return ContractDeployClosure(deploy_address, contract_db);
}

Hash
ContractDBProxy::create_contract(std::shared_ptr<const Contract> contract)
{
    Hash h = hash_xdr(*contract);
    new_contracts[h] = std::make_unique<const MeteredContract>(contract);
    return h;
}

ContractCreateClosure
ContractDBProxy::push_create_contract(wasm_api::Hash const& h, std::shared_ptr<const MeteredContract> contract)
{
    return ContractCreateClosure(h, contract, contract_db);
}

bool
ContractDBProxy::push_updates_to_db(TransactionRewind& rewind)
{
    assert_not_committed();
    is_committed = true;
    
    for (auto const& [addr, hash] : new_deployments) {
        auto res = push_deploy_contract(addr, hash);
        if (res) {
            rewind.add(std::move(*res));
        } else {
            return false;
        }
    }

    for (auto const& [hash, script] : new_contracts) {
        rewind.add(push_create_contract(hash, script));
    }
    return true;
}

wasm_api::Script
ContractDBProxy::get_script(const wasm_api::Hash& address) const
{
    auto it = new_deployments.find(address);

    if (it == new_deployments.end()) {
        return wasm_api::null_script;
    }

    auto const& script_hash = it->second;
    
    auto s_it = new_contracts.find(script_hash);
    if (s_it == new_contracts.end()) {
        return contract_db.get_script_by_hash(script_hash);
    }
    return {s_it->second->data(), s_it -> second-> size() };
}

void
ContractDBProxy::assert_not_committed() const
{
    if (is_committed) {
        throw std::runtime_error(
            "ContractDBProxy::assert_not_commited() failed");
    }
}

} // namespace scs
