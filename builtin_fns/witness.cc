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

#include "transaction_context/execution_context.h"

namespace scs {

BUILTIN_DECL(void)::scs_get_witness(uint64_t wit_idx,
                            uint32_t out_offset,
                            uint32_t max_len)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_get_witness(max_len));

    auto& runtime = *tx_ctx.get_current_runtime();

    auto const& wit = tx_ctx.get_witness(wit_idx);

    runtime.write_to_memory(wit.value, out_offset, max_len);
}

BUILTIN_DECL(uint32_t)::scs_get_witness_len(uint64_t wit_idx)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_get_witness_len);

    return tx_ctx.get_witness(wit_idx).value.size();
}

} // namespace scs
