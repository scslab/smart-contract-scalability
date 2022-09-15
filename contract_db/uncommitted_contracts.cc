#include "contract_db/uncommitted_contracts.h"
#include "contract_db/contract_db.h"

#include "crypto/hash.h"

namespace scs {

bool
UncommittedContracts::deploy_contract_to_address(
    wasm_api::Hash const& addr,
    wasm_api::Hash const& script_hash)
{
    std::lock_guard lock(mtx);

    auto [_, insertion_happened] = new_deployments.emplace(addr, script_hash);

    return insertion_happened;
}

void
UncommittedContracts::undo_deploy_contract_to_address(
    wasm_api::Hash const& addr)
{
    std::lock_guard lock(mtx);

    new_deployments.erase(addr);
}

void
UncommittedContracts::add_new_contract(
    std::shared_ptr<const Contract> new_contract)
{
    Hash h = hash_xdr(*new_contract);
    std::lock_guard lock(mtx);

    new_contracts.emplace(h, new_contract);
}

void
UncommittedContracts::clear()
{
	new_contracts.clear();
	new_deployments.clear();
}

void
UncommittedContracts::commit(ContractDB& contract_db)
{
    for (auto const& [hash, script] : new_contracts) {
        contract_db.commit_contract_to_db(hash, script);
    }

    for (auto const& [addr, hash] : new_deployments) {
        contract_db.commit_registration(addr, hash);
    }
    clear();
}

} // namespace scs
