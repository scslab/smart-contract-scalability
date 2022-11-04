#include "transaction_context/transaction_context.h"
#include "transaction_context/global_context.h"

#include "storage_proxy/transaction_rewind.h"

#include "crypto/hash.h"

#include <wasm_api/error.h>

namespace scs {

TransactionContext::TransactionContext(SignedTransaction const& tx,
                                       Hash const& tx_hash,
                                       GlobalContext& global_context,
                                       BlockContext& block_context_)
    : invocation_stack()
    , runtime_stack()
    , tx(tx)
    , tx_hash(tx_hash)
    , gas_used(0)
    , block_context(block_context_)
    , return_buf()
    , tx_results(new TransactionResults())
    , storage_proxy(global_context.state_db, block_context_.modified_keys_list)
    , contract_db_proxy(global_context.contract_db)
{}

wasm_api::WasmRuntime*
TransactionContext::get_current_runtime()
{
    if (runtime_stack.size() == 0) {
        throw std::runtime_error(
            "no active runtime during get_current_runtime() call");
    }
    return runtime_stack.back();
}

const MethodInvocation&
TransactionContext::get_current_method_invocation() const
{
    return invocation_stack.back();
}

void
TransactionContext::pop_invocation_stack()
{
    invocation_stack.pop_back();
    runtime_stack.pop_back();
}

void
TransactionContext::push_invocation_stack(wasm_api::WasmRuntime* runtime,
                                          MethodInvocation const& invocation)
{
    invocation_stack.push_back(invocation);
    runtime_stack.push_back(runtime);
}

AddressAndKey
TransactionContext::get_storage_key(InvariantKey const& key) const
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

const Address&
TransactionContext::get_msg_sender() const
{
    if (invocation_stack.size() <= 1) {
        throw wasm_api::HostError("no sender on root tx");
        //return tx.sender;
    }
    return invocation_stack[invocation_stack.size() - 2].addr;
}

const Hash&
TransactionContext::get_src_tx_hash() const
{
    return tx_hash;
}

Hash
TransactionContext::get_invoked_tx_hash() const
{
    return hash_xdr(tx.tx);
}

uint32_t
TransactionContext::get_num_deployable_contracts() const
{
    return tx.tx.contracts_to_deploy.size();
}

uint64_t 
TransactionContext::get_block_number() const
{
    return block_context.block_number;
}

const Contract&
TransactionContext::get_deployable_contract(uint32_t index) const
{
    if (index >= tx.tx.contracts_to_deploy.size()) {
        throw std::runtime_error("invalid contract access");
    }
    return tx.tx.contracts_to_deploy[index];
}

bool
TransactionContext::push_storage_deltas()
{
    assert_not_committed();
    committed_to_statedb = true;

    TransactionRewind rewind;

    if (!storage_proxy.push_deltas_to_statedb(rewind)) {
        return false;
    }

    if (!contract_db_proxy.push_updates_to_db(rewind)) {
        return false;
    }

    rewind.commit();
    storage_proxy.commit();

    return true;
}

WitnessEntry const&
TransactionContext::get_witness(uint64_t wit_idx) const
{
    for (auto const& w : tx.witnesses)
    {
        if (w.key == wit_idx)
        {
            return w;
        }
    }
    throw wasm_api::HostError("witness not found");
}

} // namespace scs
