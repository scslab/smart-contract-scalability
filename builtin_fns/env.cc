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

#include "groundhog/types.h"

namespace scs {

BUILTIN_INSTANTIATE;


BUILTIN_DECL(int32_t)::memcmp(uint32_t lhs, uint32_t rhs, uint32_t sz)
{
    auto& tx_ctx = GET_TEC;
    
    tx_ctx.consume_gas(gas_memcmp(sz));

    auto& runtime = *tx_ctx.get_current_runtime();

    return runtime.memcmp(lhs, rhs, sz);
}

BUILTIN_DECL(uint32_t)::memset(uint32_t p, uint32_t val, uint32_t sz)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_memset(sz));

    auto& runtime = *tx_ctx.get_current_runtime();

    return runtime.memset(p, static_cast<uint8_t>(val & 0xFF), sz);
}

BUILTIN_DECL(uint32_t)::memcpy(uint32_t dst, uint32_t src, uint32_t sz)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_memcpy(sz));

    auto& runtime = *tx_ctx.get_current_runtime();
    return runtime.safe_memcpy(dst, src, sz);
}

BUILTIN_DECL(uint32_t)::strnlen(uint32_t str, uint32_t max_len)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_strnlen(max_len));

    auto& runtime = *tx_ctx.get_current_runtime();
    return runtime.safe_strlen(str, max_len);
}

} // namespace scs
