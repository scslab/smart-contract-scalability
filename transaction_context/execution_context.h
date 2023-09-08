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

#include "transaction_context/global_context.h"

#include <wasm_api/wasm_api.h>

#include "xdr/transaction.h"

namespace scs {

template<typename TransactionContext_t>
class ThreadlocalContextStore;
template<typename TransactionContext_t>
class BuiltinFns;


struct MethodInvocation;

template<typename TransactionContext_t>
class ExecutionContext
{
    wasm_api::WasmContext wasm_context;

    std::map<Address, std::unique_ptr<wasm_api::WasmRuntime>> active_runtimes;

    std::unique_ptr<TransactionContext_t> tx_context;

    std::unique_ptr<TransactionResults> results_of_last_tx;

    ExecutionContext();

    friend class ThreadlocalContextStore<TransactionContext_t>;
    friend class BuiltinFns<TransactionContext_t>;

    // should only be used by builtin fns
    void invoke_subroutine(MethodInvocation const& invocation);

    auto& get_transaction_context()
    {
      if (!tx_context) {
        throw std::runtime_error(
            "can't get ctx outside of active tx execution");
      }
      return *tx_context;
    }

    void extract_results();
    void reset();

  public:
    template<typename BlockContext, typename GlobalContext>
    TransactionStatus execute(Hash const& tx_hash,
                              SignedTransaction const& tx,
                              BlockContext& block_context,
                              GlobalContext& global_context,
                              std::optional<NondeterministicResults> res = std::nullopt);

    std::vector<TransactionLog> const& get_logs();

    ~ExecutionContext();
};

} // namespace scs
