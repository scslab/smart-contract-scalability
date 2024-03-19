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
    , persistence("contract_log/")
{}

void
ContractDB::assert_not_uncommitted_modifications() const
{
    if (has_uncommitted_modifications.load(std::memory_order_relaxed))
    {
        throw std::runtime_error("contractdb has uncommitted modifications assert fail");
    }
}


VerifiedScriptView
ContractDB::get_script_by_address(Address const& addr) const
{
    auto const* contract = addresses_to_contracts_map.get_value_nolocks(addr);
    if (contract == nullptr)
    {
        return VerifiedScriptView{ .bytes = nullptr, .len = 0};
    }

    auto const* metered_script_out = contract -> contract.get();

    if (metered_script_out == nullptr)
    {
        throw std::runtime_error("invalid script stored within ContractDB!");
    }

    return metered_script_out->to_view();
}

VerifiedScriptView
ContractDB::get_script_by_hash(const Hash& hash) const
{
    auto it = hashes_to_contracts_map.find(hash);
    if (it == hashes_to_contracts_map.end()) {
        return {nullptr, 0};
    }
    if (it -> second.get() == nullptr) {
        throw std::runtime_error("invalid script stored within ContractDB");
    }
    return it -> second.get()->to_view();//{ it->second.get()->data(), it -> second.get()->size() };
}

void
ContractDB::commit_contract_to_db(Hash const& contract_hash,
                                  verified_script_ptr_t new_contract)
{
    //std::printf("commit contract to db %s\n", debug::array_to_str(contract_hash).c_str());
    auto res = hashes_to_contracts_map.emplace(contract_hash, new_contract);

    if (!res.second) {
        throw std::runtime_error("failed to commit contract (contract at this "
                                 "hash already existed)");
    }
}

void
ContractDB::commit_registration(Address const& new_address,
                                Hash const& contract_hash)
{
    // throws if there's some error (with groundhog, not a sandbox)
    // and contract_hash doesn't exist
    auto ptr = hashes_to_contracts_map.at(contract_hash);

    if (addresses_to_contracts_map.get_value_nolocks(new_address) != nullptr)
    {
        throw std::runtime_error("double registration of a contract");
    }

    // Relies on UncommittedContractsDB ensuring we never commit two things to the same address in one block
    addresses_to_contracts_map.insert(new_address, value_t(ptr));

    // This is threadsafe
    // TODO(geoff): why did I bother making persistence threadsafe
    // when this function is only ever called in a sequential loop
    persistence.log_deploy(new_address, contract_hash);
}

bool
ContractDB::deploy_contract_to_address(Address const& addr,
                                       Hash const& script_hash)
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
    Hash const& h, 
    verified_script_ptr_t new_contract)
{
    has_uncommitted_modifications.store(true, std::memory_order_relaxed);
    uncommitted_contracts.add_new_contract(h, new_contract);
    persistence.log_create(h, new_contract);
}

bool
ContractDB::check_address_open_for_deployment(const Hash& addr) const
{
    return addresses_to_contracts_map.get_value_nolocks(addr) == nullptr;
//    return addresses_to_contracts_map.find(addr)
//           == addresses_to_contracts_map.end();
}

void
ContractDB::commit(uint32_t timestamp)
{
    uncommitted_contracts.commit(*this);
    has_uncommitted_modifications.store(false, std::memory_order_relaxed);
    persistence.write(timestamp);
}

void
ContractDB::rewind()
{
    uncommitted_contracts.clear();
    has_uncommitted_modifications.store(false, std::memory_order_relaxed);
    persistence.nowrite();
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
