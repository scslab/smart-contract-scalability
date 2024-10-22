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
#include "contract_db/contract_utils.h"

#include "crypto/hash.h"

#include "storage_proxy/transaction_rewind.h"

#include "debug/debug_utils.h"

namespace scs {

ContractCreateClosure::ContractCreateClosure(
    Hash const& h, 
    metered_contract_ptr_t contract,
    std::shared_ptr<const Contract> unmetered_contract,
    ContractDB& contract_db)
    : h(h)
    , contract(contract)
    , unmetered_contract(unmetered_contract)
    , contract_db(contract_db)
{}

ContractCreateClosure::ContractCreateClosure(ContractCreateClosure&& other)
    : h(other.h)
    , contract(other.contract)
    , unmetered_contract(other.unmetered_contract)
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
        contract_db.add_new_uncommitted_contract(h, std::move(contract), std::move(unmetered_contract));
    }
}

ContractDeployClosure::ContractDeployClosure(
    const Address& deploy_address,
    const Hash& contract_hash,
    ContractDB& contract_db)
    : deploy_address(deploy_address)
    , contract_hash(contract_hash)
    , contract_db(contract_db)
{}

void
ContractDeployClosure::commit()
{
    contract_db.deploy_contract_to_address(deploy_address, contract_hash);
}

bool
ContractDBProxy::check_contract_exists(const Hash& contract_hash) const
{
    if (contract_db.check_committed_contract_exists(contract_hash)) {
        return true;
    }
    return new_contracts.find(contract_hash) != new_contracts.end();
}

std::optional<Address>
ContractDBProxy::deploy_contract(const Address& sender_address,
                                 const Hash& contract_hash,
                                 uint64_t nonce)
{
    auto deploy_address = compute_contract_deploy_address(sender_address, contract_hash, nonce);

    if (!check_contract_exists(contract_hash)) {
        return std::nullopt;
    }
    if (new_deployments.find(deploy_address) != new_deployments.end()) {
        return std::nullopt;
    }

    new_deployments[deploy_address] = contract_hash;
    return {deploy_address};
}

void
ContractDBProxy::deploy_contract_at_specific_address(const Address& deploy_address,
                                                     const Hash& contract_hash)
{
    if (!check_contract_exists(contract_hash)) {
        throw std::runtime_error("invalid deploy at specific address");
    }
    if (new_deployments.find(deploy_address) != new_deployments.end()) {
        throw std::runtime_error("invalid deploy at specific address");
    }

    new_deployments[deploy_address] = contract_hash;
}


ContractDeployClosure
ContractDBProxy::push_deploy_contract(const Address& deploy_address,
                                      const Hash& contract_hash) const
{
    return ContractDeployClosure(deploy_address, contract_hash, contract_db);
}

Hash
ContractDBProxy::create_contract(std::shared_ptr<const Contract> contract)
{
    // This is where gas metering, or verification, or whatever other checks
    // on new contracts should take place
    Hash h = hash_xdr(*contract);
    new_contracts[h] = std::make_pair(std::make_shared<const MeteredContract>(contract), contract);
    return h;
}

ContractCreateClosure
ContractDBProxy::push_create_contract(
    Hash const& h, std::pair<metered_contract_ptr_t, std::shared_ptr<const Contract>> const& contract)
{
    return ContractCreateClosure(h, contract.first, contract.second, contract_db);
}

void
ContractDBProxy::push_updates_to_db(TransactionRewind& rewind)
{
    assert_not_committed();
    is_committed = true;
    
    for (auto const& [addr, hash] : new_deployments) {
        rewind.add(push_deploy_contract(addr, hash));
    }

    for (auto const& [hash, script] : new_contracts) {
        rewind.add(push_create_contract(hash, script));
    }
}

RunnableScriptView
ContractDBProxy::get_script(const Address& address) const
{
    auto it = new_deployments.find(address);

    if (it == new_deployments.end()) {
        std::printf("get here\n");
        return contract_db.get_script_by_address(address);
    }

    auto const& script_hash = it->second;
    
    auto s_it = new_contracts.find(script_hash);
    if (s_it == new_contracts.end()) {
        std::printf("get there\n");
        return contract_db.get_script_by_hash(script_hash);
    }
    std::printf("get end\n");
    return s_it-> second.first->to_view();//{s_it->second.first->data(), s_it -> second.first -> size() };
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
