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

#include "builtin_fns/builtin_fns.h"
#include "builtin_fns/gas_costs.h"

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/transaction_context.h"
#include "transaction_context/execution_context.h"

#include "contract_db/contract_utils.h"

#include "wasm_api/error.h"

#include "xdr/storage.h"

#include <cstdint>
#include <vector>

namespace scs {

BUILTIN_INSTANTIATE;


BUILTIN_DECL(void)::scs_create_contract(uint32_t contract_index, uint32_t hash_out
                                /* out_len = 32 */)
{
    auto& tx_ctx = GET_TEC;

    uint32_t num_contracts = tx_ctx.get_num_deployable_contracts();
    if (num_contracts <= contract_index) {
        throw wasm_api::HostError("Invalid deployable contract access");
    }

    std::shared_ptr<const Contract> contract = std::make_shared<const Contract>(
        tx_ctx.get_deployable_contract(contract_index));

    tx_ctx.consume_gas(gas_create_contract(contract -> size()));

    Hash h = tx_ctx.contract_db_proxy.create_contract(contract);

    auto& runtime = *tx_ctx.get_current_runtime();

    runtime.write_to_memory(h, hash_out, h.size());
}

BUILTIN_DECL(void)::scs_deploy_contract(uint32_t hash_offset, /* hash_len = 32 */
                                uint64_t nonce,
                                uint32_t out_address_offset /* addr_len = 32 */)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_deploy_contract);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto contract_hash
        = runtime.template load_from_memory_to_const_size_buf<Hash>(
            hash_offset);

    auto deploy_addr = compute_contract_deploy_address(tx_ctx.get_self_addr(), contract_hash, nonce);

    if (!tx_ctx.contract_db_proxy.deploy_contract(deploy_addr, contract_hash)) {
        throw wasm_api::HostError("failed to deploy contract");
    }

    runtime.write_to_memory(deploy_addr, out_address_offset, deploy_addr.size());
}

} // namespace scs
