#include "contract_db/contract_db_proxy.h"

#include "contract_db/contract_db.h"

#include "crypto/hash.h"

#include "storage_proxy/transaction_rewind.h"

#include "debug/debug_utils.h"

namespace scs {

ContractCreateClosure::ContractCreateClosure(
    std::shared_ptr<const Contract> contract,
    ContractDB& contract_db)
    : contract(contract)
    , contract_db(contract_db)
{}

ContractCreateClosure::ContractCreateClosure(ContractCreateClosure&& other)
    : contract(other.contract)
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
        contract_db.add_new_uncommitted_contract(std::move(contract));
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
    new_contracts[h] = contract;
    return h;
}

ContractCreateClosure
ContractDBProxy::push_create_contract(std::shared_ptr<const Contract> contract)
{
    return ContractCreateClosure(contract, contract_db);
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
        rewind.add(push_create_contract(script));
    }
    return true;
}

const std::vector<uint8_t>*
ContractDBProxy::get_script(const wasm_api::Hash& address) const
{
    auto it = new_deployments.find(address);

    if (it == new_deployments.end()) {
        return nullptr;
        //return contract_db.get_script(address, nullptr);
    }

    auto const& script_hash = it->second;
    
    auto s_it = new_contracts.find(script_hash);
    if (s_it == new_contracts.end()) {
        return contract_db.get_script_by_hash(script_hash);
    }
    return s_it->second.get();
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
