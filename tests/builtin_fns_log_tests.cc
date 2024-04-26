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

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/global_context.h"

#include "crypto/hash.h"
#include "utils/load_wasm.h"

#include "test_utils/deploy_and_commit_contractdb.h"

namespace scs {

TEST_CASE("test log", "[builtin]")
{
    test::DeferredContextClear defer;
    
    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;

    auto c = load_wasm_from_file("lfi_contracts/test_log.lfi");
    auto h = hash_xdr(*c);

    test::deploy_and_commit_contractdb(script_db, h, c);

    ExecutionContext<TxContext> exec_ctx(scs_data_structures.engine);

    BlockContext block_context(0);

    auto exec_success = [&](const Hash& tx_hash, const SignedTransaction& tx) {
        REQUIRE(exec_ctx.execute(tx_hash, tx, scs_data_structures, block_context)
                == TransactionStatus::SUCCESS);
    };

    auto exec_fail = [&](const Hash& tx_hash, const SignedTransaction& tx) {
        REQUIRE(exec_ctx.execute(tx_hash, tx, scs_data_structures, block_context)
                != TransactionStatus::SUCCESS);
    };

    auto make_tx = [&](TransactionInvocation const& invocation)
        -> std::pair<Hash, SignedTransaction> {
        Transaction tx(
            invocation, UINT64_MAX, 1, xdr::xvector<Contract>());

        SignedTransaction stx;
        stx.tx = tx;

        return { hash_xdr(stx), stx };
    };

    SECTION("log hardcoded")
    {
        TransactionInvocation invocation(h, 0, xdr::opaque_vec<>());

        auto [h, tx] = make_tx(invocation);

        exec_success(h, tx);

        auto const& logs = exec_ctx.get_logs();

        REQUIRE(logs.size() == 1);

        if (logs.size() >= 1) {
            REQUIRE(logs[0].size() == 8);
            REQUIRE(logs[0]
                    == std::vector<uint8_t>{
                        0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA });
        }
    }
    SECTION("log calldata")
    {
        TransactionInvocation invocation(
            h,
            1,
            xdr::opaque_vec<>{
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 });

        auto [h, tx] = make_tx(invocation);
        exec_success(h, tx);

        auto const& logs = exec_ctx.get_logs();

        REQUIRE(logs.size() == 1);

        if (logs.size() >= 1) {
            REQUIRE(logs[0].size() == 8);
            REQUIRE(logs[0]
                    == std::vector<uint8_t>{
                        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 });
        }
    }

    SECTION("log twice")
    {
        TransactionInvocation invocation(
            h,
            2,
            xdr::opaque_vec<>{
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 });

        auto [h, tx] = make_tx(invocation);
        exec_success(h, tx);

        auto const& logs = exec_ctx.get_logs();

        REQUIRE(logs.size() == 2);

        if (logs.size() >= 2) {
            REQUIRE(logs[0].size() == 4);
            REQUIRE(logs[0] == std::vector<uint8_t>{ 0x00, 0x01, 0x02, 0x03 });
            REQUIRE(logs[1] == std::vector<uint8_t>{ 0x04, 0x05, 0x06, 0x07 });
        }
    }
}

} // namespace scs
