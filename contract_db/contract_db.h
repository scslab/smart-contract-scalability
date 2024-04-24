#pragma once

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

#include "xdr/types.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>

#include "contract_db/uncommitted_contracts.h"

#include <utils/non_movable.h>

#include <mtt/trie/merkle_trie.h>

#include "metering_ffi/metered_contract.h"

#include "contract_db/contract_persistence.h"
#include "contract_db/runnable_script.h"
#include "contract_db/types.h"

namespace scs {

class ContractDB
    : public utils::NonMovableOrCopyable
{
    struct value_struct
    {
        verified_contract_ptr_t contract;

        /**
         * One might consider swapping this for a hash of the data, instead.
         * The merkle trie library stores all of the intermediate node hashes,
         * so (because these values never change) a contract only has copy_data
         * called on it once.  Hashing it before copying it over
         * would trade the cost of a hash against the cost of a memcpy,
         * but regardless both are one-time costs and at the moment
         * are not particularly significant in experiments.
         */
        void copy_data(std::vector<uint8_t>& buf) const
        {
            if (contract)
            {
                auto view = contract->to_view();
                buf.insert(buf.end(), view.data, view.data + view.len);
            }
        }
    };

    using prefix_t = trie::ByteArrayPrefix<sizeof(Hash)>;
    using metadata_t
        = trie::CombinedMetadata<trie::SizeMixin>;
    using value_t
        = value_struct;

    using contract_map_t
        = trie::MerkleTrie<prefix_t, value_t, metadata_t>;

    contract_map_t addresses_to_contracts_map;
    std::map<Hash, verified_contract_ptr_t> hashes_to_contracts_map;

    UncommittedContracts uncommitted_contracts;

    std::atomic<bool> has_uncommitted_modifications = false;

    AsyncPersistContracts persistence;

    void assert_not_uncommitted_modifications() const;

    friend class UncommittedContracts;

    void commit_contract_to_db(Hash const& contract_hash,
                               verified_contract_ptr_t new_contract);

    void commit_registration(Address const& new_address,
                             Hash const& contract_hash);

    friend class ContractCreateClosure;
    friend class ContractDeployClosure;
    friend class ContractDBProxy;

    void undo_deploy_contract_to_address(Address const& addr)
    {
        uncommitted_contracts.undo_deploy_contract_to_address(addr);
    }

    bool __attribute__((warn_unused_result))
    deploy_contract_to_address(Address const& addr,
                               Hash const& script_hash);

    void add_new_uncommitted_contract(
        Hash const& h,
        verified_contract_ptr_t new_contract,
        std::shared_ptr<const Contract> new_unmetered_contract);

    RunnableScriptView get_script_by_hash(const Hash& hash) const;

    RunnableScriptView get_script_by_address(
        Address const& addr) const;


  public:
    ContractDB();

    bool check_address_open_for_deployment(const Address& addr) const;

    bool check_committed_contract_exists(const Hash& contract_hash) const;

    void commit(uint32_t timestamp);

    void rewind();

    Hash hash();
};

} // namespace scs
