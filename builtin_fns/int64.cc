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

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/method_invocation.h"

#include "wasm_api/error.h"

#include "xdr/storage.h"
#include "xdr/storage_delta.h"

#include <cstdint>
#include <vector>

namespace scs {

BUILTIN_INSTANTIATE;

/**
 * Implements the Nonnegative Integer primitive.
 * 
 * Values stored on the ledger are 64-bit _signed_ integers.
 * Transactions modify these with `set_add(x,y)` operations.
 * This sets the value of the integer to x, and then adds y to it.
 * 
 * All transactions in a block must set the value to the same
 * integer, if multiple transactions call set_add(x, y_i) on the same value.
 * 
 * After a block of transactions, if x is nonnegative,
 * then the runtime guarantees that $x + \sum_i y_i$ must be nonnegative.
 * If x is set to a negative integer, then each $y_i$ must be nonnegative.
 * 
 * The choice to allow negative values of x is in the name of adding flexibility.
 * This choice makes it slightly easier for accounts to implement some applications,
 * such as debts (incurred e.g. by accumulated interest), and it does not 
 * make any other applications more difficult
 * (a contract can always wrap calls to set_add by an assertion that x>=0).
 **/
BUILTIN_DECL(void)::scs_nonnegative_int64_set_add(uint32_t key_offset,
                                          /* key_len = 32 */
                                          int64_t set_value,
                                          int64_t delta)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_nonnegative_int64_set_add);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    tx_ctx.storage_proxy.nonnegative_int64_set_add(
        addr_and_key, set_value, delta);
}

BUILTIN_DECL(void)::scs_nonnegative_int64_add(uint32_t key_offset,
                                      /* key_len = 32 */
                                      int64_t delta)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_nonnegative_int64_add);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);

    EXEC_TRACE("int64 add %lld to key %s",
               delta,
               debug::array_to_str(addr_and_key).c_str());

    tx_ctx.storage_proxy.nonnegative_int64_add(
        addr_and_key, delta);
}

// return 1 if key exists, 0 else
BUILTIN_DECL(uint32_t)::scs_nonnegative_int64_get(uint32_t key_offset,
                                      /* key_len = 32 */
                                      uint32_t output_offset
                                      /* output_len = 8 */)
{
    auto& tx_ctx = GET_TEC;
    tx_ctx.consume_gas(gas_nonnegative_int64_get);

    auto& runtime = *tx_ctx.get_current_runtime();

    auto key
        = runtime.template load_from_memory_to_const_size_buf<InvariantKey>(
            key_offset);

    auto addr_and_key = tx_ctx.get_storage_key(key);
    auto const& res = tx_ctx.storage_proxy.get(addr_and_key);

    if (!res) {
        return 0;
    }

    if (res->body.type() != ObjectType::NONNEGATIVE_INT64) {
        throw wasm_api::HostError("type mismatch in raw mem get");
    }

    runtime.write_to_memory(res->body.nonnegative_int64(), output_offset);
    return 1;
}

} // namespace scs
