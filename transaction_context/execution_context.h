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

#include "xdr/transaction.h"

#include "lficpp/lficpp.h"

#include <utils/time.h>

namespace scs {

struct MethodInvocation;

class SandboxCache {
  std::vector<std::unique_ptr<DeClProc>> procs;
  size_t free_ptr = 0;
  LFIGlobalEngine& global_engine;
  void* ctxp;

public:

  SandboxCache(LFIGlobalEngine& global_engine,
    void* ctxp)
    : procs()
    , free_ptr(0)
    , global_engine(global_engine)
    , ctxp(ctxp)
    {}

  DeClProc* 
  get_new_proc() {
    if (free_ptr >= procs.size())
    {
      std::unique_ptr<DeClProc> proc = std::make_unique<DeClProc>(ctxp, global_engine);
      if (!(*proc)) {
        return nullptr;
      }
      procs.emplace_back(std::move(proc));
    }
    auto* out = procs[free_ptr].get();
    free_ptr ++;
    return out;
  }

  void reset() {
    free_ptr = 0;
  }
};

template<typename TransactionContext_t>
class ExecutionContext : utils::NonMovableOrCopyable
{
    SandboxCache proc_cache;

    std::unique_ptr<TransactionContext_t> tx_context;

    std::unique_ptr<TransactionResults> results_of_last_tx;

    RpcAddressDB* addr_db;

    utils::detail::time_point basept;

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

    RpcAddressDB& get_address_db() {
      if (addr_db == nullptr) {
        throw std::runtime_error("null addr db");
      }
      return *addr_db;
    }

    uint64_t syscall_handler(uint64_t callno, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

  public:
    template<typename BlockContext, typename GlobalContext>
    TransactionStatus execute(Hash const& tx_hash,
                              SignedTransaction const& tx,
                              GlobalContext& global_context,
                              BlockContext& block_context,
                              std::optional<NondeterministicResults> res = std::nullopt);

    std::vector<TransactionLog> const& get_logs();

    ExecutionContext(LFIGlobalEngine& engine);

    ~ExecutionContext();

    static uint64_t static_syscall_handler(void* self, 
      uint64_t callno, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
    {
      return reinterpret_cast<ExecutionContext*>(self)->syscall_handler(callno, arg0, arg1, arg2, arg3, arg4, arg5);
    }
};

} // namespace scs
