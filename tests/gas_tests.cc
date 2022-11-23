#include <catch2/catch_test_macros.hpp>

#include "transaction_context/global_context.h"

#include "crypto/hash.h"
#include "test_utils/deploy_and_commit_contractdb.h"
#include "utils/load_wasm.h"
#include "utils/make_calldata.h"

#include "debug/debug_utils.h"

#include "threadlocal/threadlocal_context.h"

using namespace scs;

TEST_CASE("gas metering", "[gas]")
{
    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;

    auto c = load_wasm_from_file("cpp_contracts/test_gas_limit.wasm");

    auto h = hash_xdr(*c);

    test::deploy_and_commit_contractdb(script_db, h, c);

    ThreadlocalContextStore::make_ctx(scs_data_structures);
    test::DeferredContextClear defer;

    std::unique_ptr<BlockContext> block_context
        = std::make_unique<BlockContext>(0);

    auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

    auto make_spin_tx = [&](uint64_t duration,
                            uint64_t gas_limit,
                            bool success = true) -> Hash {
        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(h, 0, make_calldata(duration));

        Transaction tx = Transaction(
            invocation, gas_limit, gas_bid, xdr::xvector<Contract>());
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

    auto make_loop_tx = [&](uint64_t gas_limit) -> Hash {
        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(h, 1, make_calldata());

        Transaction tx = Transaction(
            invocation, gas_limit, gas_bid, xdr::xvector<Contract>());
        SignedTransaction stx;
        stx.tx = tx;

        auto hash = hash_xdr(stx);

        REQUIRE(exec_ctx.execute(hash, stx, *block_context)
                != TransactionStatus::SUCCESS);

        return hash;
    };

    SECTION("short") { make_spin_tx(3, 10000); }
    SECTION("long") { make_spin_tx(10000, 10000, false); }
    SECTION("low gas") { make_spin_tx(0, 1, false); }
    SECTION("loop short") { make_loop_tx(1); }
    SECTION("loop long") { make_loop_tx(100000); }
}
