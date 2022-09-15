#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "transaction_context/transaction_context.h"

#include <wasm_api/wasm_api.h>

#include "xdr/transaction.h"

#include "transaction_context/global_context.h"

namespace scs {

class BlockContext;

class ExecutionContext
{

    wasm_api::WasmContext wasm_context;
    GlobalContext& scs_data_structures;

    std::map<Address, std::unique_ptr<wasm_api::WasmRuntime>> active_runtimes;

    std::unique_ptr<TransactionContext> tx_context;

    std::unique_ptr<TransactionResults> results_of_last_tx;

    ExecutionContext(GlobalContext& scs_data_structures)
        : wasm_context(scs_data_structures.contract_db, MAX_STACK_BYTES)
        , scs_data_structures(scs_data_structures)
        , active_runtimes()
        , tx_context(nullptr)
        , results_of_last_tx(nullptr)
    {}

    friend class ThreadlocalContextStore;
    friend class BuiltinFns;

    // should only be used by builtin fns
    void invoke_subroutine(MethodInvocation const& invocation);

    TransactionContext& get_transaction_context();

    void extract_results();
    void reset();

  public:
    TransactionStatus execute(Hash const& tx_hash,
                              Transaction const& tx,
                              BlockContext& block_context);

    std::vector<std::vector<uint8_t>> const& get_logs();
};

} // namespace scs
