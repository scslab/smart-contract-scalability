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

#include "contract_db/uncommitted_contracts.h"
#include "contract_db/contract_db.h"

#include "crypto/hash.h"

namespace scs {

void
UncommittedContracts::deploy_contract_to_address(
    Address const& addr,
    Hash const& script_hash)
{
    std::lock_guard lock(mtx);

    new_deployments.emplace(addr, script_hash);
}

void
UncommittedContracts::undo_deploy_contract_to_address(
    Address const& addr)
{
    std::lock_guard lock(mtx);

    new_deployments.erase(addr);
}

void
UncommittedContracts::add_new_contract(
    Hash const& h,
    verified_contract_ptr_t new_contract)
{
    std::lock_guard lock(mtx);

    auto [_, inserted] = new_contracts.emplace(h, new_contract);

    if (!inserted)
    {
        throw std::runtime_error("invalid add_new_contract (already existed)");
    }
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
