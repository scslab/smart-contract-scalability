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

#include <catch2/catch_test_macros.hpp>

#include "transaction_context/global_context.h"
#include "transaction_context/execution_context.h"
#include "groundhog/types.h"

#include "phase/phases.h"

#include "crypto/hash.h"
#include "test_utils/deploy_and_commit_contractdb.h"
#include "utils/load_wasm.h"
#include "utils/make_calldata.h"

#include "debug/debug_utils.h"

#include "threadlocal/threadlocal_context.h"

using namespace scs;

TEST_CASE("hashset manipulation test contract", "[sdk][hashset]")
{
    test::DeferredContextClear<GroundhogTxContext> defer;

    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;

    auto c = load_wasm_from_file("cpp_contracts/test_hashset_manipulation.wasm");

    auto h = hash_xdr(*c);

    test::deploy_and_commit_contractdb(script_db, h, c);

    std::unique_ptr<GroundhogBlockContext> block_context
        = std::make_unique<GroundhogBlockContext>(0);


    auto make_tx = [&](uint32_t round, bool success = true) -> Hash {

        ThreadlocalTransactionContextStore<GroundhogTxContext>::make_ctx();
        auto& exec_ctx = ThreadlocalTransactionContextStore<GroundhogTxContext>::get_exec_ctx();

        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(h, round, make_calldata());

        Transaction tx = Transaction(
            invocation, UINT64_MAX, gas_bid, xdr::xvector<Contract>());
        SignedTransaction stx;
        stx.tx = tx;

        auto hash = hash_xdr(stx);

        if (success)
        {
            REQUIRE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
                    == TransactionStatus::SUCCESS);
        }
        else
        {
            REQUIRE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
                    != TransactionStatus::SUCCESS);
        }

        return hash;
    };

    auto check_valid = [&](const Hash& tx_hash) {
        REQUIRE(block_context->tx_set.contains_tx(tx_hash));
    };

    auto finish_block = [&]() {
        phase_finish_block(scs_data_structures, *block_context);
    };

    auto advance_block = [&] ()
    {
        block_context = std::make_unique<GroundhogBlockContext>(block_context -> block_number + 1);
    };

    std::printf("start work section\n");

    SECTION("good manips")
    {
        auto tx0 = make_tx(0);

        finish_block();
        check_valid(tx0);
        advance_block();

        auto tx2 = make_tx(2);
        
        finish_block();
        check_valid(tx2);
        advance_block();
    }

    SECTION("bad manips")
    {
        // prevent the degenerate case of contract calling clear(),
        // and then inserting something that would be cleared -- by common sense,
        // the contract should see this write, but then this write will never be materialized after
        // the block.  This would just be weird.
        SECTION("insert after clear")
        {
            make_tx(1, false);
        }
    }

}

