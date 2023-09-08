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
#include "transaction_context/method_invocation.h"

#include <cstdint>
#include <vector>

#include "groundhog/types.h"

namespace scs {

BUILTIN_INSTANTIATE;

BUILTIN_DECL(uint32_t)::scs_has_key(uint32_t key_offset
                        /* key_len = 32 */)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_has_key);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    auto res = tx_ctx.storage_proxy.get(addr_and_key);

    return res.has_value();
}

} // namespace scs
