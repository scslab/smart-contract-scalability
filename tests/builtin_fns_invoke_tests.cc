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
#include "test_utils/deploy_and_commit_contractdb.h"
#include "utils/load_wasm.h"
#include "utils/make_calldata.h"

#include "state_db/modified_keys_list.h"

using xdr::operator==;

namespace scs {

TEST_CASE("test invoke", "[builtin]")
{
    test::DeferredContextClear<TxContext> defer;

    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;

    auto c1 = load_wasm_from_file("cpp_contracts/test_log.wasm");
    auto h1 = hash_xdr(*c1);

    auto c2
        = load_wasm_from_file("cpp_contracts/test_redirect_call.wasm");
    auto h2 = hash_xdr(*c2);

    test::deploy_and_commit_contractdb(script_db, h1, c1);
    test::deploy_and_commit_contractdb(script_db, h2, c2);

    ThreadlocalTransactionContextStore<TxContext>::make_ctx();

    auto& exec_ctx = ThreadlocalTransactionContextStore<TxContext>::get_exec_ctx();

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

    SECTION("msg sender self")
    {
        TransactionInvocation invocation(h1, 4, xdr::opaque_vec<>());

        auto [h, tx] = make_tx(invocation);
        exec_success(h, tx);

        auto const& logs = exec_ctx.get_logs();

        REQUIRE(logs.size() == 1);

        if (logs.size() >= 1) {
            REQUIRE(logs[0].size() == 32);
            REQUIRE(memcmp(logs[0].data(), h1.data(), 32) == 0);
        }
    }

    SECTION("msg sender self fails on base call")
    {
        TransactionInvocation invocation(h1, 3, xdr::opaque_vec<>());

        auto [h, tx] = make_tx(invocation);
        exec_fail(h, tx);
    }

    SECTION("msg sender other")
    {
        struct calldata_t
        {
            Address callee;
            uint32_t method;
        };

        calldata_t data{ .callee = h1, .method = 3 };

        TransactionInvocation invocation(h2, 0, make_calldata(data));

        auto [h, tx] = make_tx(invocation);
        exec_success(h, tx);

        auto const& logs = exec_ctx.get_logs();

        REQUIRE(logs.size() == 1);

        if (logs.size() >= 1) {
            REQUIRE(logs[0].size() == 32);
            REQUIRE(memcmp(logs[0].data(), h2.data(), 32) == 0);
        }
    }

    SECTION("invoke self")
    {
        struct calldata_t
        {
            Address callee;
            uint32_t method;
        };

        calldata_t data{ .callee = h2, .method = 1 };

        TransactionInvocation invocation(h2, 0, make_calldata(data));

        auto [h, tx] = make_tx(invocation);
        exec_success(h, tx);

        auto const& logs = exec_ctx.get_logs();

        REQUIRE(logs.size() == 1);

        if (logs.size() >= 1) {
            REQUIRE(logs[0].size() == 1);
            REQUIRE(logs[0] == std::vector<uint8_t>{ 0xFF });
        }
    }

    SECTION("invoke self with reentrance guard")
    {
        struct calldata_t
        {
            Address callee;
            uint32_t method;
        };

        calldata_t data{ .callee = h2, .method = 2 };

        TransactionInvocation invocation(h2, 0, make_calldata(data));

        auto [h, tx] = make_tx(invocation);
        exec_fail(h, tx);
    }

    SECTION("invoke other")
    {
        struct calldata_t
        {
            Address callee;
            uint32_t method;
        };

        calldata_t data{ .callee = h1, .method = 0 };

        TransactionInvocation invocation(h2, 0, make_calldata(data));

        auto [h, tx] = make_tx(invocation);
        exec_success(h, tx);

        auto const& logs = exec_ctx.get_logs();

        REQUIRE(logs.size() == 1);
    }

    SECTION("invoke insufficient calldata")
    {
        TransactionInvocation invocation(h1, 1, xdr::opaque_vec<>());

        auto [h, tx] = make_tx(invocation);
        exec_fail(h, tx);
    }

    SECTION("invoke nonexistent")
    {
        struct calldata_t
        {
            Address callee;
            uint32_t method;
        };

        Address empty_addr;

        calldata_t data{ .callee = empty_addr, .method = 1 };

        TransactionInvocation invocation(h2, 0, make_calldata(data));

        auto [h, tx] = make_tx(invocation);
        exec_fail(h, tx);
    }
}

} // namespace scs
