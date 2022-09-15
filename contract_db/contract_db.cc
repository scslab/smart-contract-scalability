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

const std::vector<uint8_t>*
ContractDB::get_script(wasm_api::Hash const& addr,
                       const wasm_api::script_context_t& context) const
{
    const ContractDBProxy* proxy = static_cast<const ContractDBProxy*>(context);

    if (proxy != nullptr) {
        auto const* res = proxy->get_script(addr);

        if (res) {
            return res;
        }
    }
    
    auto it = hashes_to_contracts_map.find(addr);
    if (it == hashes_to_contracts_map.end()) {
        return nullptr;
    }
    return it->second.get();
}

void
ContractDB::commit_contract_to_db(wasm_api::Hash const& contract_hash,
                                  std::shared_ptr<const Contract> new_contract)
{
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
    auto res = addresses_to_contracts_map.emplace(new_address, ptr);

    if (!res.second) {
        throw std::runtime_error("failed to register contract to address "
                                 "(contract at this addr already existed)");
    }
}

bool
ContractDB::deploy_contract_to_address(wasm_api::Hash const& addr,
                                       wasm_api::Hash const& script_hash)
{
    if (!check_address_open_for_deployment(addr)) {
        return false;
    }

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
    std::shared_ptr<const Contract> new_contract)
{
    uncommitted_contracts.add_new_contract(new_contract);
}

bool
ContractDB::check_address_open_for_deployment(const wasm_api::Hash& addr) const
{
    return addresses_to_contracts_map.find(addr)
           == addresses_to_contracts_map.end();
}

void
ContractDB::commit()
{
    uncommitted_contracts.commit(*this);
}

} // namespace scs
