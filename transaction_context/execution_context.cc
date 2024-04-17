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

#include <sys/mman.h>

#include "lficpp/signal_handler.h"

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
{
    signal_init();
}


EC_DECL(void)::invoke_subroutine(MethodInvocation const& invocation)
{
    auto* runtime = tx_context->get_runtime_by_addr(invocation.addr);
    if (runtime == nullptr) {
        CONTRACT_INFO("creating new runtime for contract at %s",
                      debug::array_to_str(invocation.addr).c_str());

        runtime = proc_cache.get_new_proc();

        if (runtime == nullptr)
        {
            throw HostError("failed to allocate new proc");
        }

        auto script = tx_context -> get_contract_db_proxy().get_script(invocation.addr);

        if (runtime -> set_program(script.bytes, script.len) != 0) {
            throw std::runtime_error("program nexist at target address");
        }

        tx_context->add_active_runtime(invocation.addr, runtime);
    }

    tx_context->push_invocation_stack(runtime, invocation);

    // TODO: all the invocation data, set registers, etc
    int code = runtime -> run(invocation.method_name, invocation.calldata);
    if (code != 0)
    {
        throw HostError("invocation failed");
    }

    tx_context->pop_invocation_stack();
}

enum {
    SYS_EXIT  = 500,
    SYS_WRITE = 501,
    SYS_SBRK  = 502,
    SYS_LSEEK = 503,
    SYS_READ  = 504,
    SYS_CLOSE = 505,

    LFIHOG_LOG = 600,
    LFIHOG_INVOKE=601,

    WRITE_MAX = 1024,
};

EC_DECL(uint64_t)::syscall_handler(uint64_t callno, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    int64_t ret = -1;
    std::printf("SYSCALL: %ld\n", callno);
    LFIProc* p = tx_context->get_current_runtime();
    try {
        switch (callno) {
        case SYS_EXIT:
            p->exit(arg0);
            break;
        case SYS_WRITE:
            arg2 = arg2 > WRITE_MAX ? WRITE_MAX : arg2;
            ret = write(1, (const char*) p->addr(arg1), (size_t) arg2);
            break;
        case SYS_SBRK: {
            ret = p -> sbrk(arg0);
            break;
        }
        case SYS_LSEEK:
            break;
        case SYS_READ:
            break;
        case SYS_CLOSE:
            break;
        case LFIHOG_LOG:{
            // arg0: ptr
            // arg1: len
            uintptr_t ptr = arg0;
            uint32_t len = arg1;
            if (p -> is_readable(p->addr(ptr), len))
            {
                tx_context->tx_results->add_log(TransactionLog(p->addr(ptr), p->addr(ptr + len)));
            }
            ret = 0;
            break;
        }
        case LFIHOG_INVOKE:{
            // arg0: address
            // arg1: method
            // arg2: calldata_ptr
            // arg3: calldata_len
            // arg4
            uintptr_t addr = arg0;
            uint32_t method = arg1;
            uintptr_t calldata_addr = arg2;
            uint32_t calldata_len = arg3;
            uintptr_t return_addr = arg4;
            uint32_t return_len = arg5;

            Address address;
            std::vector<uint8_t> calldata;

            if (p -> is_readable(p->addr(addr), 32))
            {
                std::memcpy(address.data(), reinterpret_cast<uint8_t*>(p->addr(addr)), 32);                
            } else {
                ret = -1;
                break;
            }

            if (p -> is_readable(p->addr(calldata_addr), calldata_len))
            {
                calldata.insert(
                    calldata.end(),
                    reinterpret_cast<uint8_t*>(p->addr(calldata_addr)),
                    reinterpret_cast<uint8_t*>(p->addr(calldata_addr + calldata_len)));
            }

            invoke_subroutine(MethodInvocation(address, method, std::move(calldata)));

            return_len = std::min<uint32_t>(return_len, tx_context->return_buf.size());

            if (return_len > 0)
            {
                if (p -> is_writable(p->addr(return_addr), return_len))
                {
                    std::memcpy(
                        reinterpret_cast<uint8_t*>(p->addr(return_addr)), 
                        reinterpret_cast<uint8_t*>(tx_context->return_buf.data()), 
                        return_len);
                }
            }

            tx_context->return_buf.clear();
            ret = 0;
            break;
        }
        default:
            std::printf("invalid syscall: %ld\n", callno);
            std::abort();
        }
    } catch (HostError& e) {
        std::printf("tx failed %s\n", e.what());
        CONTRACT_INFO("Execution error: %s", e.what());
        tx_context -> get_current_runtime() -> exit(1);
        std::unreachable();
    } catch (...) {
        std::printf("unrecoverable error!\n");
        std::abort();
    }
    return (uint64_t) ret;
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
