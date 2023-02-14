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

#include <cstdint>
#include <memory>
#include <optional>

#include "transaction_context/transaction_context.h"

#include <wasm_api/wasm_api.h>

#include "xdr/transaction.h"

namespace scs {

class BlockContext;
class GlobalContext;

class ExecutionContext
{
    wasm_api::WasmContext wasm_context;
    GlobalContext& scs_data_structures;

    std::map<Address, std::unique_ptr<wasm_api::WasmRuntime>> active_runtimes;

    std::unique_ptr<TransactionContext> tx_context;

    std::unique_ptr<TransactionResults> results_of_last_tx;

    ExecutionContext(GlobalContext& scs_data_structures);

    friend class ThreadlocalContextStore;
    friend class BuiltinFns;

    // should only be used by builtin fns
    void invoke_subroutine(MethodInvocation const& invocation);

    TransactionContext& get_transaction_context();

    void extract_results();
    void reset();

  public:
    TransactionStatus execute(Hash const& tx_hash,
                              SignedTransaction const& tx,
                              BlockContext& block_context,
                              std::optional<NondeterministicResults> res = std::nullopt);

    std::vector<TransactionLog> const& get_logs();

    ~ExecutionContext();
};

} // namespace scs
