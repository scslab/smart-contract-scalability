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