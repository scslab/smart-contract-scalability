#pragma once

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
                              BlockContext& block_context);

    std::vector<TransactionLog> const& get_logs();

    ~ExecutionContext();
};

} // namespace scs
