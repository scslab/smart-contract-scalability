#pragma once

#include "xdr/types.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>

#include <wasm_api/wasm_api.h>

#include "contract_db/uncommitted_contracts.h"

#include <utils/non_movable.h>

#include <mtt/trie/merkle_trie.h>

namespace scs {

class ContractDB
    : public wasm_api::ScriptDB
    , public utils::NonMovableOrCopyable
{

    static_assert(
        sizeof(wasm_api::Hash) == sizeof(Address),
        "mismatch between addresses in scs and addresses in wasm api");

    struct value_struct
    {
        std::shared_ptr<const Contract> contract;

        void copy_data(std::vector<uint8_t>& buf) const
        {
            if (contract)
            {
                buf.insert(buf.end(), contract->begin(), contract->end());
            }
        }
    };

    using prefix_t = trie::ByteArrayPrefix<sizeof(wasm_api::Hash)>;
    using metadata_t
        = trie::CombinedMetadata<trie::SizeMixin>;
    using value_t
        = value_struct;

    using contract_map_t
        = trie::MerkleTrie<prefix_t, value_t, metadata_t>;
    //    = std::map<wasm_api::Hash, std::shared_ptr<const Contract>>;

    contract_map_t addresses_to_contracts_map;
    std::map<wasm_api::Hash, std::shared_ptr<const Contract>> hashes_to_contracts_map;

    UncommittedContracts uncommitted_contracts;

    std::atomic<bool> has_uncommitted_modifications = false;

    void assert_not_uncommitted_modifications() const;

    friend class UncommittedContracts;

    void commit_contract_to_db(wasm_api::Hash const& contract_hash,
                               std::shared_ptr<const Contract> new_contract);

    void commit_registration(wasm_api::Hash const& new_address,
                             wasm_api::Hash const& contract_hash);

    friend class ContractCreateClosure;
    friend class ContractDeployClosure;
    friend class ContractDBProxy;

    void undo_deploy_contract_to_address(wasm_api::Hash const& addr)
    {
        uncommitted_contracts.undo_deploy_contract_to_address(addr);
    }

    bool __attribute__((warn_unused_result))
    deploy_contract_to_address(wasm_api::Hash const& addr,
                               wasm_api::Hash const& script_hash);

    void add_new_uncommitted_contract(
        std::shared_ptr<const Contract> new_contract);

  public:
    ContractDB();

    const std::vector<uint8_t>* get_script(
        wasm_api::Hash const& addr,
        const wasm_api::script_context_t&) const override final;

    bool check_address_open_for_deployment(const wasm_api::Hash& addr) const;

    bool check_committed_contract_exists(const Hash& contract_hash) const;

    void commit();

    void rewind();

    Hash hash();
};

} // namespace scs
