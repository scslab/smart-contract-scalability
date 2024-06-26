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

template<typename TransactionContext_t>
class ExecutionContext : public utils::NonMovableOrCopyable
{
    wasm_api::WasmContext wasm_context;

    std::map<Address, std::unique_ptr<wasm_api::WasmRuntime>> active_runtimes;

    std::unique_ptr<TransactionContext_t> tx_context;

    std::unique_ptr<TransactionResults> results_of_last_tx;

    RpcAddressDB* addr_db;

    void invoke_subroutine(MethodInvocation const& invocation);

    auto& get_transaction_context()
    {
      if (!tx_context) {
        throw std::runtime_error(
            "can't get ctx outside of active tx execution");
      }
      return *tx_context;
    }

    int64_t syscall_handler(uint64_t callno, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

    static 
    int64_t static_syscall_handler(void* self, uint64_t callno, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
    {
      if (self == nullptr) {
        throw std::runtime_error("cannot invoke static syscall on nullptr self");
      }
      return reinterpret_cast<ExecutionContext*>(self) -> syscall_handler(callno, arg0, arg1, arg2, arg3, arg4, arg5);
    }

    void gas_handler(uint64_t gas);

    static
    void static_gas_handler(void* self, uint64_t gas)
    {
      if (self == nullptr) {
        throw std::runtime_error("cannot invoke static syscall on nullptr self");
      }
      reinterpret_cast<ExecutionContext*>(self) -> gas_handler(gas);
    }

    int32_t env_memcmp(uint32_t lhs, uint32_t rhs, uint32_t sz);
    uint32_t env_memset(uint32_t ptr, uint32_t val, uint32_t len);
    uint32_t env_memcpy(uint32_t dst, uint32_t src, uint32_t len);
    uint32_t env_strnlen( uint32_t ptr, uint32_t max_len);

    static
    int32_t 
    static_env_memcmp(void* self, uint32_t lhs, uint32_t rhs, uint32_t sz) {
      if (self == nullptr) {
        throw std::runtime_error("invalid self ptr in env");
      }
      return reinterpret_cast<ExecutionContext*>(self) -> env_memcmp(lhs, rhs, sz);
    }

    static
    uint32_t 
    static_env_memset(void* self, uint32_t ptr, uint32_t val, uint32_t len)
    { 
      if (self == nullptr) {
        throw std::runtime_error("invalid self ptr in env");
      }
      return reinterpret_cast<ExecutionContext*>(self) -> env_memset(ptr, val, len);
    }

    static
    uint32_t
    static_env_memcpy(void* self, uint32_t dst, uint32_t src, uint32_t len)
    {
      if (self == nullptr) {
        throw std::runtime_error("invalid self ptr in env");
      }
      return reinterpret_cast<ExecutionContext*>(self) -> env_memcpy(dst, src, len);
    }

    static
    uint32_t
    static_env_strnlen(void* self, uint32_t ptr, uint32_t max_len)
    {
      if (self == nullptr) {
        throw std::runtime_error("invalid self ptr in env");
      }
      return reinterpret_cast<ExecutionContext*>(self) -> env_strnlen(ptr, max_len);
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
    ExecutionContext();

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
