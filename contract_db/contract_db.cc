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

#include "contract_db/contract_db.h"

#include "xdr/types.h"

#include "transaction_context/global_context.h"

#include "contract_db/contract_db_proxy.h"

#include "debug/debug_utils.h"

namespace scs {

ContractDB::ContractDB()
    : addresses_to_contracts_map()
    , hashes_to_contracts_map()
    , uncommitted_contracts()
{}

void
ContractDB::assert_not_uncommitted_modifications() const
{
    if (has_uncommitted_modifications.load(std::memory_order_relaxed))
    {
        throw std::runtime_error("contractdb has uncommitted modifications assert fail");
    }
}


wasm_api::Script
ContractDB::get_script_by_address(wasm_api::Hash const& addr) const
{
    /*
    const ContractDBProxy* proxy = static_cast<const ContractDBProxy*>(context);

    if (proxy != nullptr) {
        auto res = proxy->get_script(addr);

        if (res.data) {
            return res;
        }
    } */

    auto const* contract = addresses_to_contracts_map.get_value_nolocks(addr);
    if (contract == nullptr)
    {
        return wasm_api::null_script;
    }

    auto const* metered_script_out = contract -> contract.get();

    if (metered_script_out == nullptr || metered_script_out -> data() == nullptr)
    {
        throw std::runtime_error("invalid script stored within ContractDB!");
    }

    return { metered_script_out -> data(), metered_script_out -> size()};
}

wasm_api::Script
ContractDB::get_script_by_hash(const wasm_api::Hash& hash) const
{
    auto it = hashes_to_contracts_map.find(hash);
    if (it == hashes_to_contracts_map.end()) {
        return {nullptr, 0};
    }
    return { it->second.get()->data(), it -> second.get()->size() };
}

void
ContractDB::commit_contract_to_db(wasm_api::Hash const& contract_hash,
                                  metered_contract_ptr_t new_contract)
{
    //std::printf("commit contract to db %s\n", debug::array_to_str(contract_hash).c_str());
    auto res = hashes_to_contracts_map.emplace(contract_hash, new_contract);

    if (!res.second) {
        throw std::runtime_error("failed to commit contract (contract at this "
                                 "hash already existed)");
    }
}

void
ContractDB::commit_registration(wasm_api::Hash const& new_address,
                                wasm_api::Hash const& contract_hash)
{
    auto ptr = hashes_to_contracts_map.at(contract_hash);

/*
    std::printf("committing registration of contract %s to address %s\n",
        debug::array_to_str(contract_hash).c_str(),
        debug::array_to_str(new_address).c_str());
*/

    /*value_t const* ptr = hashes_to_contracts_map.get_value_nolocks(new_address);
    if (ptr == nullptr)
    {
        throw std::runtime_error("failed to find contract in commit_registration");
    } */

    if (addresses_to_contracts_map.get_value_nolocks(new_address) != nullptr)
    {
        throw std::runtime_error("double registration of a contract");
    }

    addresses_to_contracts_map.insert(new_address, value_t(ptr));
}

bool
ContractDB::deploy_contract_to_address(wasm_api::Hash const& addr,
                                       wasm_api::Hash const& script_hash)
{
    if (!check_address_open_for_deployment(addr)) {
        return false;
    }

    has_uncommitted_modifications.store(true, std::memory_order_relaxed);

    return uncommitted_contracts.deploy_contract_to_address(addr, script_hash);
}

bool
ContractDB::check_committed_contract_exists(const Hash& contract_hash) const
{
    return hashes_to_contracts_map.find(contract_hash)
           != hashes_to_contracts_map.end();
}

void
ContractDB::add_new_uncommitted_contract(
    wasm_api::Hash const& h, 
    std::shared_ptr<const MeteredContract> new_contract)
{
    has_uncommitted_modifications.store(true, std::memory_order_relaxed);
    uncommitted_contracts.add_new_contract(h, new_contract);
}

bool
ContractDB::check_address_open_for_deployment(const wasm_api::Hash& addr) const
{
    return addresses_to_contracts_map.get_value_nolocks(addr) == nullptr;
//    return addresses_to_contracts_map.find(addr)
//           == addresses_to_contracts_map.end();
}

void
ContractDB::commit()
{
    uncommitted_contracts.commit(*this);
    has_uncommitted_modifications.store(false, std::memory_order_relaxed);
}

void
ContractDB::rewind()
{
    uncommitted_contracts.clear();
    has_uncommitted_modifications.store(false, std::memory_order_relaxed);
}

Hash
ContractDB::hash()
{
    assert_not_uncommitted_modifications();

    Hash out;
    addresses_to_contracts_map.hash(out);
    return out;

}

} // namespace scs
