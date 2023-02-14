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

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

#include <cstdint>
#include <vector>

namespace scs {

void
BuiltinFns::scs_print_debug(uint32_t addr, uint32_t len)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();

    auto& runtime = *tx_ctx.get_current_runtime();

    auto log
        = runtime.template load_from_memory<std::vector<uint8_t>>(addr, len);

    CONTRACT_INFO("print: addr %lu len %lu value %s",
                  addr,
                  len,
                  debug::array_to_str(log).c_str());
}

void
BuiltinFns::scs_print_c_str(uint32_t addr, uint32_t len)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();

    auto& runtime = *tx_ctx.get_current_runtime();

    auto log
        = runtime.template load_from_memory<std::vector<uint8_t>>(addr, len);
    log.push_back('\0');

    std::printf("print: %s\n", log.data());
}

void
BuiltinFns::scs_log(uint32_t log_offset, uint32_t log_len)
{
    auto& tx_ctx
        = ThreadlocalContextStore::get_exec_ctx().get_transaction_context();
    tx_ctx.consume_gas(gas_log(log_len));

    auto& runtime = *tx_ctx.get_current_runtime();

    CONTRACT_INFO("Logging offset=%lu len=%lu", log_offset, log_len);

    auto log = runtime.template load_from_memory<std::vector<uint8_t>>(
        log_offset, log_len);

    tx_ctx.tx_results->add_log(TransactionLog(log.begin(), log.end()));
}

} // namespace scs
