#include "transaction_context/execution_context.h"
#include "transaction_context/global_context.h"
#include "transaction_context/transaction_results.h"

#include "threadlocal/threadlocal_context.h"

#include "crypto/hash.h"

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

#include "builtin_fns/builtin_fns.h"

#include "storage_proxy/transaction_rewind.h"

#include "transaction_context/global_context.h"

#include "utils/defer.h"

#include <utils/time.h>

namespace scs {

ExecutionContext::ExecutionContext(GlobalContext& scs_data_structures)
    : wasm_context(scs_data_structures.contract_db, MAX_STACK_BYTES)
    , scs_data_structures(scs_data_structures)
    , active_runtimes()
    , tx_context(nullptr)
    , results_of_last_tx(nullptr)
{}


void
ExecutionContext::invoke_subroutine(MethodInvocation const& invocation)
{
    auto iter = active_runtimes.find(invocation.addr);
    if (iter == active_runtimes.end()) {
        CONTRACT_INFO("creating new runtime for contract at %s",
                      debug::array_to_str(invocation.addr).c_str());

        auto timestamp = utils::init_time_measurement();

        auto runtime_instance = wasm_context.new_runtime_instance(
            invocation.addr,
            static_cast<const void*>(&(tx_context->get_contract_db_proxy())));
        if (!runtime_instance) {
            throw wasm_api::HostError("cannot find target address");
        }

        std::printf("launch time: %lf\n", utils::measure_time(timestamp));

        active_runtimes.emplace(invocation.addr, std::move(runtime_instance));
        BuiltinFns::link_fns(*active_runtimes.at(invocation.addr));

        std::printf("link time: %lf\n", utils::measure_time(timestamp));
    }

    auto* runtime = active_runtimes.at(invocation.addr).get();

    tx_context->push_invocation_stack(runtime, invocation);

    runtime->template invoke<void>(
        invocation.get_invocable_methodname().c_str());

    tx_context->pop_invocation_stack();
}

TransactionStatus
ExecutionContext::execute(Hash const& tx_hash,
                          SignedTransaction const& tx,
                          BlockContext& block_context,
                          std::optional<NondeterministicResults> nondeterministic_res)
{
    if (tx_context) {
        throw std::runtime_error("one execution at one time");
    }

    MethodInvocation invocation(tx.tx.invocation);

    tx_context = std::make_unique<TransactionContext>(
        tx, tx_hash, scs_data_structures, block_context, nondeterministic_res);

    defer d{ [this]() {
        extract_results();
        reset();
    } };

    try {
        invoke_subroutine(invocation);
    } catch (wasm_api::HostError& e) {
        CONTRACT_INFO("Execution error: %s", e.what());
        // txs.template invalidate<TransactionFailurePoint::COMPUTE>(tx_hash);
        return TransactionStatus::FAILURE;
    } catch (...) {
        std::printf("unrecoverable error!\n");
        std::abort();
    }

    if (!tx_context->push_storage_deltas()) {
        return TransactionStatus::FAILURE;
    }

    block_context.tx_set.add_transaction(tx_hash, tx, tx_context -> tx_results->get_results().ndeterministic_results);

    return TransactionStatus::SUCCESS;
}

void
ExecutionContext::extract_results()
{
    results_of_last_tx = tx_context->extract_results();
}

void
ExecutionContext::reset()
{
    // nothing to do to clear wasm_context
    active_runtimes.clear();
    tx_context.reset();
}

TransactionContext&
ExecutionContext::get_transaction_context()
{
    if (!tx_context) {
        throw std::runtime_error(
            "can't get ctx outside of active tx execution");
    }
    return *tx_context;
}

std::vector<TransactionLog> const&
ExecutionContext::get_logs()
{

    if (!results_of_last_tx) {
        throw std::runtime_error("can't get logs before execution");
    }
    return results_of_last_tx->logs;
}

ExecutionContext::~ExecutionContext()
{
  if (tx_context)
  {
    std::printf("cannot destroy without unwinding inflight tx\n");
    std::fflush(stdout);
    std::terminate();
  }
}


} // namespace scs
