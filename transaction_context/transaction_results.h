#pragma once

#include "xdr/transaction.h"
#include "xdr/rpc.h"

#include <cstdint>
#include <vector>

namespace scs {

class TransactionResultsFrame
{
    TransactionResults results;

    const bool validating;

    uint32_t rpc_idx = 0, log_idx = 0;

public:

    TransactionResultsFrame(NondeterministicResults res)
        : results()
        , validating(true)
        {
            results.ndeterministic_results = res;
        }
    TransactionResultsFrame()
        : results()
        , validating(false)
        {}

    bool is_validating() const
    {
        return validating;
    }

    void add_log(TransactionLog log);

    void add_rpc_result(RpcResult result);
    RpcResult get_next_rpc_result();

    TransactionResults get_results() {
        return results;
    }
};

} // namespace scs