#include "builtin_fns/builtin_fns.h"

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/method_invocation.h"

#include "wasm_api/error.h"

#include "xdr/storage.h"

#include <cstdint>
#include <vector>

namespace scs {

void
BuiltinFns::scs_create_contract(uint32_t contract_index, uint32_t hash_out
                                /* out_len = 32 */)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();

    uint32_t num_contracts = tx_ctx.get_num_deployable_contracts();
    if (num_contracts <= contract_index) {
        throw wasm_api::HostError("Invalid deployable contract access");
    }

    std::shared_ptr<const Contract> contract = std::make_shared<const Contract>(
        tx_ctx.get_deployable_contract(contract_index));

    Hash h = tx_ctx.contract_db_proxy.create_contract(contract);

    auto& runtime = *tx_ctx.get_current_runtime();

    runtime.write_to_memory(h, hash_out, h.size());
}

void
BuiltinFns::scs_deploy_contract(uint32_t hash_offset, /* hash_len = 32 */
                                uint32_t address_offset /* addr_len = 32 */)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    auto& runtime = *tx_ctx.get_current_runtime();

    auto contract_hash
        = runtime.template load_from_memory_to_const_size_buf<Hash>(
            hash_offset);

    auto deploy_addr
        = runtime.template load_from_memory_to_const_size_buf<Address>(
            address_offset);

    if (!tx_ctx.contract_db_proxy.deploy_contract(deploy_addr, contract_hash)) {
        throw wasm_api::HostError("failed to deploy contract");
    }
}

} // namespace scs
