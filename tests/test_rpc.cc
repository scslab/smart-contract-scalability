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

#include "phase/phases.h"

#include "crypto/hash.h"
#include "test_utils/deploy_and_commit_contractdb.h"
#include "utils/load_wasm.h"
#include "utils/make_calldata.h"

#include "debug/debug_utils.h"

#include "threadlocal/threadlocal_context.h"

#include "xdr/types.h"

#include "rpc_server/echo_server.h"
#include "rpc_server/server_runner.h"

#include "config.h"

using namespace scs;

using xdr::operator==;

TEST_CASE("simulated echo rpc", "[rpc]")
{
	test::DeferredContextClear<GroundhogTxContext> defer;

    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;

    auto c = load_wasm_from_file("cpp_contracts/test_rpc.wasm");

    auto h = hash_xdr(*c);

    test::deploy_and_commit_contractdb(script_db, h, c);

    ThreadlocalTransactionContextStore<GroundhogTxContext>::make_ctx();

    auto& exec_ctx = ThreadlocalTransactionContextStore<GroundhogTxContext>::get_exec_ctx();

    std::unique_ptr<GroundhogBlockContext> block_context
        = std::make_unique<GroundhogBlockContext>(0);

    /**
     * Note to future:
     * There's some weird thing going on (possibly in the test macro, I would guess?)
     * in which adding variable_addr causes very strange compile errors.
     * Just changing the variable name fixes it.
     */
    Hash rpcAddr = hash_xdr<uint64_t>(0);

   	auto make_ndet = [] (uint64_t return_value)
   	{
   		RpcResult res;
   		res.result.insert(
   			res.result.end(),
   			reinterpret_cast<uint8_t*>(&return_value),
   			reinterpret_cast<uint8_t*>(&return_value) + 8);
   		NondeterministicResults out;
   		out.rpc_results.push_back(res);
   		return out;
   	};

    auto make_tx_withres = [&](uint64_t calldata, uint64_t response, bool success = true) -> Hash {

        const uint64_t gas_bid = 1;

        struct rpc_calldata {
        	Hash addr;
        	uint64_t value;
        };

        TransactionInvocation invocation(h, 0, make_calldata(rpc_calldata { .addr = rpcAddr, .value = calldata }));

        Transaction tx = Transaction(
            invocation, UINT64_MAX, gas_bid, xdr::xvector<Contract>());
        SignedTransaction stx;
        stx.tx = tx;

        auto hash = hash_xdr(stx);

        if (success)
        {
            REQUIRE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context, make_ndet(response))
                    == TransactionStatus::SUCCESS);
        }
        else
        {
            REQUIRE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context, make_ndet(response))
                    != TransactionStatus::SUCCESS);
        }

        return hash;
    };

    auto make_tx_nores = [&](uint64_t calldata, bool success = true) -> Hash {

        const uint64_t gas_bid = 1;

        struct rpc_calldata {
        	Hash addr;
        	uint64_t value;
        };

        TransactionInvocation invocation(h, 0, make_calldata(rpc_calldata { .addr = rpcAddr, .value = calldata }));

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
    auto check_invalid = [&](const Hash& tx_hash) {
        REQUIRE(!block_context->tx_set.contains_tx(tx_hash));
    };

    auto finish_block = [&]() {
        phase_finish_block(scs_data_structures, *block_context);
    };

    ServerRunner echo_server(std::make_unique<EchoServer>(), "localhost:9000");

    scs_data_structures.address_db.add_mapping(rpcAddr, RpcAddress { .addr = "localhost:9000" });

    SECTION("one good response")
    {
    	auto tx1 = make_tx_withres(100, 100);
    	finish_block();
        check_valid(tx1);
    }

    SECTION("one bad response")
    {
    	auto tx1 = make_tx_withres(100, 101, false);
    	finish_block();
        check_invalid(tx1);
    } 

    #if USE_RPC
    SECTION("one call")
    {
    	auto tx1 = make_tx_nores(100);
    	finish_block();
    	check_valid(tx1);
    }
    #endif
}



TEST_CASE("test ensure ndet results all consumed", "[rpc]")
{
    test::DeferredContextClear<GroundhogTxContext> defer;

    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;

    auto c1 = load_wasm_from_file("cpp_contracts/test_log.wasm");
    auto h1 = hash_xdr(*c1);

    test::deploy_and_commit_contractdb(script_db, h1, c1);

    ThreadlocalTransactionContextStore<GroundhogTxContext>::make_ctx();

    auto& exec_ctx = ThreadlocalTransactionContextStore<GroundhogTxContext>::get_exec_ctx();

    GroundhogBlockContext block_context(0);

    auto exec_success = [&](const Hash& tx_hash, const SignedTransaction& tx, std::optional<NondeterministicResults> res) {
        REQUIRE(exec_ctx.execute(tx_hash, tx, scs_data_structures, block_context, res)
                == TransactionStatus::SUCCESS);
    };

    auto exec_fail = [&](const Hash& tx_hash, const SignedTransaction& tx, std::optional<NondeterministicResults> res) {
        REQUIRE(exec_ctx.execute(tx_hash, tx, scs_data_structures, block_context, res)
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

    SECTION("null hypothesis case")
    {
        TransactionInvocation invocation(h1, 0, xdr::opaque_vec<>());
        auto [h, tx] = make_tx(invocation);
        exec_success(h, tx, std::nullopt);
    }

    SECTION("null hypothesis validation case")
    {
        TransactionInvocation invocation(h1, 0, xdr::opaque_vec<>());
        auto [h, tx] = make_tx(invocation);
        exec_success(h, tx, NondeterministicResults());
    }

    SECTION("bad case")
    {
        TransactionInvocation invocation(h1, 0, xdr::opaque_vec<>());
        auto [h, tx] = make_tx(invocation);
        NondeterministicResults res;
        res.rpc_results.push_back(RpcResult());
        exec_fail(h, tx, res);
    }

}