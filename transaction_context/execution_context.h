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

struct MethodInvocation;

wasm_api::HostFnStatus<void> gas_handler(wasm_api::HostCallContext* context, uint64_t gas);

template<typename TransactionContext_t>
class ExecutionContext : public utils::NonMovableOrCopyable
{
    wasm_api::WasmContext wasm_context;

    std::map<Address, std::unique_ptr<wasm_api::WasmRuntime>> active_runtimes;

    std::unique_ptr<TransactionContext_t> tx_context;

    std::unique_ptr<TransactionResults> results_of_last_tx;

    RpcAddressDB* addr_db;

    wasm_api::MeteredReturn
    invoke_subroutine(MethodInvocation const& invocation, uint64_t gas_limit);

    auto& get_transaction_context()
    {
      if (!tx_context) {
        throw std::runtime_error(
            "can't get ctx outside of active tx execution");
      }
      return *tx_context;
    }

    wasm_api::HostFnStatus<uint64_t>
    syscall_handler(wasm_api::WasmRuntime* runtime, uint64_t callno, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) noexcept;

    static 
    wasm_api::HostFnStatus<uint64_t> 
    static_syscall_handler(wasm_api::HostCallContext* host_call_context, uint64_t callno, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) noexcept
    {
      void* self = host_call_context->user_ctx;
      if (self == nullptr) {
        std::terminate();
      }
      try {
        return reinterpret_cast<ExecutionContext*>(self) -> syscall_handler(host_call_context -> runtime, callno, arg0, arg1, arg2, arg3, arg4, arg5);
      } catch(...) {
        return wasm_api::HostFnStatus<uint64_t>{std::unexpect_t{}, wasm_api::HostFnError::UNRECOVERABLE};
      }
    }

    void extract_results();
    void reset();

    RpcAddressDB& get_address_db() {
      if (addr_db == nullptr) {
        throw std::runtime_error("null addr db");
      }
      return *addr_db;
    }

  public:
    ExecutionContext(wasm_api::SupportedWasmEngine engine);

    template<typename BlockContext, typename GlobalContext>
    TransactionStatus execute(Hash const& tx_hash,
                              SignedTransaction const& tx,
                              GlobalContext& global_context,
                              BlockContext& block_context,
                              std::optional<NondeterministicResults> res = std::nullopt);

    std::vector<TransactionLog> const& get_logs();

    ~ExecutionContext();
};

} // namespace scs
