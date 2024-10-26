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

#include "transaction_context/transaction_context.h"
#include "transaction_context/global_context.h"

#include "storage_proxy/transaction_rewind.h"

#include "crypto/hash.h"

#include "transaction_context/error.h"

namespace scs {

template class TransactionContext<GlobalContext>;
template class TransactionContext<SisyphusGlobalContext>;
template class TransactionContext<GroundhogGlobalContext>;

#define TC_TEMPLATE template<typename GlobalContext_t>
#define TC_DECL TransactionContext<GlobalContext_t>

TC_TEMPLATE
TC_DECL::TransactionContext(SignedTransaction const& tx,
                                       Hash const& tx_hash,
                                       GlobalContext_t& global_context,
                                       uint64_t current_block,
                                       std::optional<NondeterministicResults> const& results)
    : invocation_stack()
    , runtime_stack()
    , tx(tx)
    , tx_hash(tx_hash)
    , current_block(current_block)
    , return_buf()
    , tx_results(results ? new TransactionResultsFrame(*results) : new TransactionResultsFrame())
    , storage_proxy(global_context.state_db)
    , contract_db_proxy(global_context.contract_db)
{}

TC_TEMPLATE
DeClProc*
TC_DECL::get_current_runtime()
{
    if (runtime_stack.size() == 0) {
        throw std::runtime_error(
            "no active runtime during get_current_runtime() call");
    }
    return runtime_stack.back();
}

TC_TEMPLATE
const MethodInvocation&
TC_DECL::get_current_method_invocation() const
{
    return invocation_stack.back();
}

TC_TEMPLATE
void
TC_DECL::pop_invocation_stack()
{
    invocation_stack.pop_back();
    runtime_stack.pop_back();
}

TC_TEMPLATE
void
TC_DECL::push_invocation_stack(DeClProc* runtime,
                                          MethodInvocation const& invocation)
{
    invocation_stack.push_back(invocation);
    runtime_stack.push_back(runtime);
}

TC_TEMPLATE
AddressAndKey
TC_DECL::get_storage_key(InvariantKey const& key) const
{
    static_assert(sizeof(Address) + sizeof(InvariantKey)
                      == sizeof(AddressAndKey),
                  "size mismatch");

    AddressAndKey out;
    auto const& cur_addr = get_current_method_invocation().addr;
    std::memcpy(out.data(), cur_addr.data(), sizeof(Address));

    std::memcpy(out.data() + sizeof(Address), key.data(), sizeof(InvariantKey));
    return out;
}

TC_TEMPLATE
const Address&
TC_DECL::get_msg_sender() const
{
    if (invocation_stack.size() <= 1) {
        throw HostError("no sender on root tx");
    }
    return invocation_stack[invocation_stack.size() - 2].addr;
}

TC_TEMPLATE
const Hash&
TC_DECL::get_src_tx_hash() const
{
    return tx_hash;
}

TC_TEMPLATE
Hash
TC_DECL::get_invoked_tx_hash() const
{
    return hash_xdr(tx.tx);
}

TC_TEMPLATE
uint32_t
TC_DECL::get_num_deployable_contracts() const
{
    return tx.tx.contracts_to_deploy.size();
}

TC_TEMPLATE
uint64_t 
TC_DECL::get_block_number() const
{
    return current_block;
}

TC_TEMPLATE
const Contract&
TC_DECL::get_deployable_contract(uint32_t index) const
{
    if (index >= tx.tx.contracts_to_deploy.size()) {
        throw std::runtime_error("invalid contract access");
    }
    return tx.tx.contracts_to_deploy[index];
}

TC_TEMPLATE
std::unique_ptr<StorageCommitment<typename TC_DECL::StateDB_t>>
TC_DECL::push_storage_deltas()
{
    assert_not_committed();
    committed_to_statedb = true;

    auto commitment = std::make_unique<StorageCommitment<StateDB_t>>(storage_proxy, tx_hash);

    if (!storage_proxy.push_deltas_to_statedb(commitment->rewind)) {
        return nullptr;
    }

    contract_db_proxy.push_updates_to_db(commitment->rewind);

    return commitment;
}

TC_TEMPLATE
WitnessEntry const&
TC_DECL::get_witness(uint64_t wit_idx) const
{
    for (auto const& w : tx.witnesses)
    {
        if (w.key == wit_idx)
        {
            return w;
        }
    }
    throw HostError("witness not found");
}

#undef TC_DECL
#undef TC_TEMPLATE

} // namespace scs


