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

#include "transaction_context/execution_context.h"
#include "transaction_context/global_context.h"
#include "transaction_context/method_invocation.h"
#include "transaction_context/transaction_results.h"
#include "transaction_context/transaction_context.h"
#include "transaction_context/error.h"

#include "threadlocal/threadlocal_context.h"

#include "crypto/hash.h"

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

#include "builtin_fns/builtin_fns.h"

#include "storage_proxy/transaction_rewind.h"

#include "transaction_context/global_context.h"

#include "utils/defer.h"

#include <utils/time.h>

#include <utility>

#define EC_DECL(ret) template<typename TransactionContext_t> ret ExecutionContext<TransactionContext_t>

namespace scs {

template class ExecutionContext<GroundhogTxContext>;
template class ExecutionContext<SisyphusTxContext>;
template class ExecutionContext<TxContext>;

EC_DECL()::ExecutionContext(LFIGlobalEngine& engine)
    : proc_cache(engine, static_cast<void*>(this))
    , tx_context(nullptr)
    , results_of_last_tx(nullptr)
    , addr_db(nullptr)
{}


EC_DECL(void)::invoke_subroutine(MethodInvocation const& invocation)
{
    auto* runtime = tx_context->get_runtime_by_addr(invocation.addr);
    if (runtime == nullptr) {
        CONTRACT_INFO("creating new runtime for contract at %s",
                      debug::array_to_str(invocation.addr).c_str());

        runtime = proc_cache.get_new_proc();

        auto script = tx_context -> get_contract_db_proxy().get_script(invocation.addr);

        if (runtime -> set_program(script.bytes, script.len) != 0) {
            throw std::runtime_error("program nexist at target address");
        }

        tx_context->add_active_runtime(invocation.addr, runtime);
    }

    tx_context->push_invocation_stack(runtime, invocation);

    // TODO: all the invocation data, set registers, etc
    int code = runtime -> run();

    if (code != 0)
    {
        throw HostError("invocation failed");
    }

    tx_context->pop_invocation_stack();
}

EC_DECL(uint64_t)::syscall_handler(uint64_t callno, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    try {

    } catch (HostError& e) {
        std::printf("tx failed %s\n", e.what());
        CONTRACT_INFO("Execution error: %s", e.what());
        tx_context -> get_current_runtime() -> exit_error();
        std::unreachable();

    } catch (...) {
        std::printf("unrecoverable error!\n");
        std::abort();
    }
    return 0;
}

template
TransactionStatus
ExecutionContext<TxContext>::execute(
    Hash const&,
    SignedTransaction const&,
    BaseGlobalContext&,
    BaseBlockContext&,
    std::optional<NondeterministicResults>);

template
TransactionStatus
ExecutionContext<GroundhogTxContext>::execute(
    Hash const&,
    SignedTransaction const&,
    GroundhogGlobalContext&,
    GroundhogBlockContext&,
    std::optional<NondeterministicResults>);

template
TransactionStatus
ExecutionContext<SisyphusTxContext>::execute(
    Hash const&,
    SignedTransaction const&,
    SisyphusGlobalContext&,
    SisyphusBlockContext&,
    std::optional<NondeterministicResults>);


template<typename TransactionContext_t>
template<typename BlockContext_t, typename GlobalContext_t>
TransactionStatus
ExecutionContext<TransactionContext_t>::execute(Hash const& tx_hash,
                          SignedTransaction const& tx,
                          GlobalContext_t& scs_data_structures,
                          BlockContext_t& block_context,
                          std::optional<NondeterministicResults> nondeterministic_res)
{
    if (tx_context) {
        throw std::runtime_error("one execution at one time");
    }

    addr_db = &scs_data_structures.address_db;

    MethodInvocation invocation(tx.tx.invocation);

    tx_context = std::make_unique<TransactionContext_t>(
        tx, tx_hash, scs_data_structures, block_context.block_number, nondeterministic_res);

    defer d{ [this]() {
        extract_results();
        reset();
    } };

    try {
        invoke_subroutine(invocation);
    } catch (HostError& e) {
	    std::printf("tx failed %s\n", e.what());
	    CONTRACT_INFO("Execution error: %s", e.what());
        return TransactionStatus::FAILURE;
    } catch (...) {
        std::printf("unrecoverable error!\n");
        std::abort();
    }

    auto storage_commitment = tx_context -> push_storage_deltas();

    if (!storage_commitment)
    {
        return TransactionStatus::FAILURE;
    }

    if (!tx_context -> tx_results -> validating_check_all_rpc_results_used())
    {
        return TransactionStatus::FAILURE;
    }

    // cannot be rewound -- this forms the threshold for commit
    if (!block_context.tx_set.try_add_transaction(tx_hash, tx, tx_context -> tx_results->get_results().ndeterministic_results))
    {
        return TransactionStatus::FAILURE;
    }

    storage_commitment -> commit(block_context.modified_keys_list);

    return TransactionStatus::SUCCESS;
}

EC_DECL(void)::extract_results()
{
    results_of_last_tx = tx_context->extract_results();
}

EC_DECL(void)::reset()
{
    proc_cache.reset();
    tx_context.reset();
    addr_db = nullptr;
}

EC_DECL(std::vector<TransactionLog> const&)::get_logs()
{
    if (!results_of_last_tx) {
        throw std::runtime_error("can't get logs before execution");
    }
    return results_of_last_tx->logs;
}

EC_DECL()::~ExecutionContext()
{
  if (tx_context)
  {
    std::printf("cannot destroy without unwinding inflight tx\n");
    std::fflush(stdout);
    std::terminate();
  }
}

} // namespace scs
