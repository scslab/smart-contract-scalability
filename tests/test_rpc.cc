#include <catch2/catch_test_macros.hpp>

#include "transaction_context/global_context.h"

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

using namespace scs;

using xdr::operator==;

TEST_CASE("simulated echo rcp", "[rpc]")
{
	test::DeferredContextClear defer;

    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;

    auto c = load_wasm_from_file("cpp_contracts/test_rpc.wasm");

    auto h = hash_xdr(*c);

    test::deploy_and_commit_contractdb(script_db, h, c);

    ThreadlocalContextStore::make_ctx(scs_data_structures);

    auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

    std::unique_ptr<BlockContext> block_context
        = std::make_unique<BlockContext>(0);

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
            REQUIRE(exec_ctx.execute(hash, stx, *block_context, make_ndet(response))
                    == TransactionStatus::SUCCESS);
        }
        else
        {
            REQUIRE(exec_ctx.execute(hash, stx, *block_context, make_ndet(response))
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
            REQUIRE(exec_ctx.execute(hash, stx, *block_context)
                    == TransactionStatus::SUCCESS);
        }
        else
        {
            REQUIRE(exec_ctx.execute(hash, stx, *block_context)
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

   // EchoServer echo(rpc_server.get_ps(), "9000");

    scs_data_structures.address_db.add_mapping(rpcAddr, RpcAddress { .addr = "localhost:9000" });

/*
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
    } */

    SECTION("one call")
    {
    	auto tx1 = make_tx_nores(100);
    	finish_block();
    	check_valid(tx1);
    }
}