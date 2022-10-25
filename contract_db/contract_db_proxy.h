#pragma once

#include <utils/non_movable.h>

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
    std::shared_ptr<const Contract> contract;
    ContractDB& contract_db;

    bool do_create = false;

  public:
    ContractCreateClosure(std::shared_ptr<const Contract> contract,
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

    std::map<wasm_api::Hash, std::shared_ptr<const Contract>> new_contracts;

    std::map<wasm_api::Hash, wasm_api::Hash> new_deployments;

    bool check_contract_exists(const Hash& contract_hash) const;

    std::optional<ContractDeployClosure> __attribute__((warn_unused_result))
    push_deploy_contract(const wasm_api::Hash& deploy_address,
                         const wasm_api::Hash& contract_hash);

    ContractCreateClosure __attribute__((warn_unused_result))
    push_create_contract(std::shared_ptr<const Contract> contract);

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

    const std::vector<uint8_t>* get_script(const wasm_api::Hash& address) const;
};

} // namespace scs