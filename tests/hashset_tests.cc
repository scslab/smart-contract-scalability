#include <catch2/catch_test_macros.hpp>

#include "transaction_context/global_context.h"

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
    test::DeferredContextClear defer;

    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;

    auto c = load_wasm_from_file("cpp_contracts/test_hashset_manipulation.wasm");

    auto h = hash_xdr(*c);

    test::deploy_and_commit_contractdb(script_db, h, c);

    ThreadlocalContextStore::make_ctx(scs_data_structures);

    auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

    std::unique_ptr<BlockContext> block_context
        = std::make_unique<BlockContext>(0);


    auto make_tx = [&](uint32_t round, bool success = true) -> Hash {

        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(h, round, make_calldata());

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

    auto finish_block = [&]() {
        phase_finish_block(scs_data_structures, *block_context);
    };

    auto advance_block = [&] ()
    {
        block_context = std::make_unique<BlockContext>(block_context -> block_number + 1);
    };

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

