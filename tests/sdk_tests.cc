#include <catch2/catch_test_macros.hpp>

#include "transaction_context/global_context.h"

#include "phase/phases.h"

#include "crypto/hash.h"
#include "test_utils/deploy_and_commit_contractdb.h"
#include "utils/load_wasm.h"
#include "test_utils/make_calldata.h"

#include "debug/debug_utils.h"

#include "threadlocal/threadlocal_context.h"

using namespace scs;

TEST_CASE("replay cache", "[sdk]")
{
    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;

    auto c = load_wasm_from_file("cpp_contracts/test_sdk.wasm");

    auto h = hash_xdr(*c);

    test::deploy_and_commit_contractdb(script_db, h, c);

    ThreadlocalContextStore::make_ctx(scs_data_structures);
    test::DeferredContextClear defer;

    auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

    std::unique_ptr<BlockContext> block_context
        = std::make_unique<BlockContext>(0);

    struct calldata_0
    {
        uint64_t nonce; // ignored in the contract
    };

    auto make_replay_tx = [&](uint64_t nonce,
                           bool success = true) -> Hash {

        calldata_0 data
        {
            .nonce = nonce
        };

        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(h, 0, test::make_calldata(data));

        Transaction tx = Transaction(
            invocation, UINT64_MAX, gas_bid, xdr::xvector<Contract>());
        SignedTransaction stx;
        stx.tx = tx;

        auto hash = hash_xdr(stx);

        if (success) {
            REQUIRE(exec_ctx.execute(hash, stx, *block_context)
                    == TransactionStatus::SUCCESS);
        } else {
            REQUIRE(exec_ctx.execute(hash, stx, *block_context)
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
    	block_context = std::make_unique<BlockContext>(block_context -> block_number + 1);
    };

    SECTION("one block")
    {
    	auto h0 = make_replay_tx(0);
    	auto h1 = make_replay_tx(1);

    	make_replay_tx(0, false);
    	make_replay_tx(1, false);

    	finish_block();

    	check_valid(h0);
    	check_valid(h1);
    }

    SECTION("overflow replay")
    {
    	for (size_t i = 0; i < 64; i++)
    	{
    		make_replay_tx(i);
    		make_replay_tx(i, false);
    	}

    	// fill up replay cache
    	make_replay_tx(500, false);

    	finish_block();

    	SECTION("no overflow next block")
    	{
    		advance_block();
    		for (size_t i = 0; i < 64; i++)
	    	{
	    		make_replay_tx(i);
	    		make_replay_tx(i, false);
	    	}
    	}
    }
}

TEST_CASE("semaphore", "[sdk]")
{
    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;

    auto c = load_wasm_from_file("cpp_contracts/test_sdk.wasm");

    auto h = hash_xdr(*c);

    test::deploy_and_commit_contractdb(script_db, h, c);

    ThreadlocalContextStore::make_ctx(scs_data_structures);
    test::DeferredContextClear defer;

    auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

    std::unique_ptr<BlockContext> block_context
        = std::make_unique<BlockContext>(0);

    struct calldata_0
    {
    };

    auto make_semaphore_tx = [&](bool success = true) -> Hash {

        calldata_0 data
        {
        };

        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(h, 1, test::make_calldata(data));

        Transaction tx = Transaction(
            invocation, UINT64_MAX, gas_bid, xdr::xvector<Contract>());
        SignedTransaction stx;
        stx.tx = tx;

        auto hash = hash_xdr(stx);

        if (success) {
            REQUIRE(exec_ctx.execute(hash, stx, *block_context)
                    == TransactionStatus::SUCCESS);
        } else {
            REQUIRE(exec_ctx.execute(hash, stx, *block_context)
                    != TransactionStatus::SUCCESS);
        }

        return hash;
    };

    auto make_semaphore2_tx = [&](bool success = true) -> Hash {

        calldata_0 data
        {
        };


        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(h, 2, test::make_calldata(data));

        Transaction tx = Transaction(
            invocation, UINT64_MAX, gas_bid, xdr::xvector<Contract>());
        
        SignedTransaction stx;
        stx.tx = tx;
        
        auto hash = hash_xdr(stx);

        if (success) {
            REQUIRE(exec_ctx.execute(hash, stx, *block_context)
                    == TransactionStatus::SUCCESS);
        } else {
            REQUIRE(exec_ctx.execute(hash, stx, *block_context)
                    != TransactionStatus::SUCCESS);
        }

        return hash;
    };

    auto finish_block = [&]() {
        phase_finish_block(scs_data_structures, *block_context);
    };

    auto advance_block = [&] ()
    {
    	block_context = std::make_unique<BlockContext>(block_context -> block_number + 1);
    };

    SECTION("one ok")
    {
    	make_semaphore_tx();

    	make_semaphore_tx(false);
    	make_semaphore_tx(false);

    	finish_block();
    }
    SECTION("next block ok")
    {
    	make_semaphore_tx();

    	finish_block();
    	advance_block();

    	make_semaphore_tx();

    	finish_block();
    	advance_block();

    	make_semaphore_tx();

    	finish_block();
    }
    SECTION("double semaphhore 2 ok")
    {
    	make_semaphore2_tx();
    	make_semaphore2_tx();
    	make_semaphore2_tx(false);
    }
}
