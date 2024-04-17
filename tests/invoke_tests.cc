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

#include "crypto/hash.h"
#include "test_utils/deploy_and_commit_contractdb.h"
#include "utils/load_wasm.h"
#include "utils/make_calldata.h"

#include "debug/debug_utils.h"

#include "threadlocal/threadlocal_context.h"

using namespace scs;



TEST_CASE("lfi calldata", "[lfi]")
{
    test::DeferredContextClear defer;

    BaseGlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;

    auto c = load_wasm_from_file("lfi_contracts/test_calldata.lfi");

    auto h = c->hash();

    test::deploy_and_commit_contractdb(script_db, h, c);

    std::unique_ptr<BaseBlockContext> block_context
        = std::make_unique<BaseBlockContext>(0);

    ExecutionContext<TxContext> exec_ctx(scs_data_structures.engine);

    const uint64_t gas_bid = 1;
    const uint32_t method = 0xAABBCCDD;

    TransactionInvocation invocation(h, method, make_calldata('a', 'b', 'c', '\0'));

    Transaction tx = Transaction(
        invocation, 100000, gas_bid, xdr::xvector<Contract>());
    SignedTransaction stx;
    stx.tx = tx;

    auto hash = hash_xdr(stx);

    REQUIRE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context) == TransactionStatus::SUCCESS);

    auto const& logs = exec_ctx.get_logs();

    REQUIRE(logs.size() == 2);

    if (logs.size() == 2) {
        REQUIRE(logs[0].size() == 4);
        REQUIRE(memcmp(logs[0].data(), reinterpret_cast<const uint8_t*>(&method), 4) == 0);
        REQUIRE(logs[1].size() == 4);
        REQUIRE(strncmp(reinterpret_cast<const char*>(logs[1].data()), "abc", 4) == 0);
    }
}

/*
TEST_CASE("lfi invoke", "[lfi]")
{
    test::DeferredContextClear defer;

    BaseGlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;
    
    auto c1 = load_wasm_from_file("lfi_contracts/test_invoke.lfi");
    auto h1 = c1->hash();

    auto c2 = load_wasm_from_file("lfi_contracts/test_calldata.lfi");
    auto h2 = c2->hash();

    test::deploy_and_commit_contractdb(script_db, h1, c1);
    test::deploy_and_commit_contractdb(script_db, h2, c2);

    std::unique_ptr<BaseBlockContext> block_context
        = std::make_unique<BaseBlockContext>(0);

    ExecutionContext<TxContext> exec_ctx(scs_data_structures.engine);

    const uint64_t gas_bid = 1;
    const uint32_t method = 1;

    TransactionInvocation invocation(h1, method, make_calldata(h2));



} */


