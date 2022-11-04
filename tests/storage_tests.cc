#include <catch2/catch_test_macros.hpp>

#include "transaction_context/global_context.h"

#include "phase/phases.h"

#include "crypto/hash.h"
#include "test_utils/deploy_and_commit_contractdb.h"
#include "utils/load_wasm.h"
#include "utils/make_calldata.h"

#include "debug/debug_utils.h"

#include "threadlocal/threadlocal_context.h"

#include "state_db/modified_keys_list.h"

#include "object/comparators.h"

using namespace scs;

using xdr::operator==;

TEST_CASE("hashset insert", "[storage]")
{
    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;
    auto& state_db = scs_data_structures.state_db;

    auto c = load_wasm_from_file("cpp_contracts/test_hashset.wasm");

    auto h = hash_xdr(*c);

    test::deploy_and_commit_contractdb(script_db, h, c);

    ThreadlocalContextStore::make_ctx(scs_data_structures);
    test::DeferredContextClear defer;

    auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

    InvariantKey k0 = hash_xdr<uint64_t>(0);

    std::unique_ptr<BlockContext> block_context
        = std::make_unique<BlockContext>(0);

    struct calldata_0
    {
        InvariantKey key;
        uint64_t value;
    };

    auto make_insert_tx = [&](
                           InvariantKey const& key,
                           uint64_t value,
                           bool success = true,
                           uint64_t gas_bid = 1) -> Hash {

        calldata_0 data
        {
            .key = key,
            .value = value
        };

        TransactionInvocation invocation(h, 0, make_calldata(data));

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

    auto check_invalid = [&](const Hash& tx_hash) {
        REQUIRE(!block_context->tx_set.contains_tx(tx_hash));
    };

    auto finish_block = [&]() {
        phase_finish_block(scs_data_structures, *block_context);
    };

    auto make_key
        = [](Address const& addr, InvariantKey const& key) -> AddressAndKey {
        AddressAndKey out;
        std::memcpy(out.data(), addr.data(), sizeof(Address));

        std::memcpy(
            out.data() + sizeof(Address), key.data(), sizeof(InvariantKey));
        return out;
    };


    SECTION("start from nexist hashset")
    {
        auto h0 = make_insert_tx(k0, 0);
        auto h1 = make_insert_tx(k0, 1);

        auto h0_1 = make_insert_tx(k0, 0, false, 2);

        REQUIRE(h0 != h0_1);

        finish_block();

        check_valid(h0);
        check_valid(h1);

        check_invalid(h0_1);

        auto hk0 = make_key(h, k0);
        auto db_val = state_db.get_committed_value(hk0);

        REQUIRE(!!db_val);
        auto& hs = db_val -> body.hash_set();

        REQUIRE(hs.hashes.size() == 2);

        REQUIRE(hs.hashes[0] > hs.hashes[1]);
    }
}

TEST_CASE("int64 storage write", "[storage]")
{
    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;
    auto& state_db = scs_data_structures.state_db;

    auto c = load_wasm_from_file("cpp_contracts/test_nn_int64.wasm");

    const auto h = hash_xdr(*c);

    test::deploy_and_commit_contractdb(script_db, h, c);

    ThreadlocalContextStore::make_ctx(scs_data_structures);
    test::DeferredContextClear defer;

    auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

    InvariantKey k0 = hash_xdr<uint64_t>(0);

    std::unique_ptr<BlockContext> block_context
        = std::make_unique<BlockContext>(0);

    struct calldata_0
    {
        InvariantKey key;
    };
    struct calldata_1
    {
        InvariantKey key;
        uint64_t value;
    };

    auto make_set_add_tx = [&](InvariantKey const& key,
                               int64_t set,
                               int64_t add,
                               bool success = true) -> Hash {
        struct calldata_1
        {
            InvariantKey key;
            int64_t set;
            int64_t delta;
        };

        calldata_1 data{ .key = key, .set = set, .delta = add };

        TransactionInvocation invocation(h, 1, make_calldata(data));

        Transaction tx = Transaction(
            invocation, UINT64_MAX, 1, xdr::xvector<Contract>());
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

    auto make_add_tx = [&](InvariantKey const& key,
                           int64_t add,
                           bool success = true) -> Hash {
        struct calldata_0
        {
            InvariantKey key;
            int64_t delta;
        };

        calldata_0 data{ .key = key, .delta = add };

        TransactionInvocation invocation(h, 0, make_calldata(data));

        Transaction tx = Transaction(
            invocation, UINT64_MAX, 1, xdr::xvector<Contract>());
        SignedTransaction stx;
        stx.tx = tx;

        auto hash = hash_xdr(stx);
        // auto hash = tx_block->insert_tx(tx);

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
        // phase_merge_delta_batches(*delta_batch);
        // phase_filter_deltas(scs_data_structures, *delta_batch, *tx_block);
        // phase_compute_state_updates(*delta_batch, *tx_block);
        phase_finish_block(scs_data_structures, *block_context);
    };

    auto make_key
        = [](Address const& addr, InvariantKey const& key) -> AddressAndKey {
        AddressAndKey out;
        std::memcpy(out.data(), addr.data(), sizeof(Address));

        std::memcpy(
            out.data() + sizeof(Address), key.data(), sizeof(InvariantKey));
        return out;
    };

    auto check_valid = [&](const Hash& tx_hash) {
        REQUIRE(block_context->tx_set.contains_tx(tx_hash));
        // REQUIRE(
        //	tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));
    };

    auto check_invalid = [&](const Hash& tx_hash) {
        REQUIRE(!block_context->tx_set.contains_tx(tx_hash));

        // REQUIRE(
        //	!tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));
    };

    SECTION("write to empty slot")
    {
        SECTION("no set")
        {
            auto h1 = make_add_tx(k0, -1, false);
            auto h2 = make_add_tx(k0, -2, false);
            auto h3 = make_add_tx(k0, 5);
            auto h4 = make_add_tx(k0, 0);

            finish_block();

            check_invalid(h1);
            check_invalid(h2);
            check_valid(h3);
            check_valid(h4);

            auto hk0 = make_key(h, k0);
            auto db_val = state_db.get_committed_value(hk0);

            REQUIRE(!!db_val);
            REQUIRE(db_val->body.nonnegative_int64() == 5);
        }

        SECTION("with set")
        {

            auto h1 = make_set_add_tx(k0, 10, -1);
            auto h2 = make_set_add_tx(k0, 10, -2);
            auto h3 = make_set_add_tx(k0, 10, 5);
            auto h4 = make_set_add_tx(k0, 10, 0);

            finish_block();

            check_valid(h1);
            check_valid(h2);
            check_valid(h3);
            check_valid(h4);

            auto hk0 = make_key(h, k0);
            auto db_val = state_db.get_committed_value(hk0);

            REQUIRE(!!db_val);
            REQUIRE(db_val->body.nonnegative_int64() == 12);
        }

        SECTION("with negative set")
        {
            auto h1 = make_set_add_tx(k0, -10, -1, false);
            auto h2 = make_set_add_tx(k0, -10, -2, false);
            auto h3 = make_set_add_tx(k0, -10, 5);
            auto h4 = make_set_add_tx(k0, -10, 0);

            finish_block();

            check_invalid(h1);
            check_invalid(h2);
            check_valid(h3);
            check_valid(h4);

            auto hk0 = make_key(h, k0);
            auto db_val = state_db.get_committed_value(hk0);

            REQUIRE(!!db_val);
            REQUIRE(db_val->body.nonnegative_int64() == -5);
        }
    }
    SECTION("with prev value")
    {

        auto h0 = make_set_add_tx(k0, 100, 0);

        finish_block();

        check_valid(h0);

        auto hk0 = make_key(h, k0);
        auto db_val = state_db.get_committed_value(hk0);

        REQUIRE(!!db_val);
        REQUIRE(db_val->body.nonnegative_int64() == 100);

        block_context.reset(new BlockContext(1));

        SECTION("without set")
        {
            auto h1 = make_add_tx(k0, -10);
            auto h2 = make_add_tx(k0, -20);
            auto h3 = make_add_tx(k0, 5);
            auto h4 = make_add_tx(k0, 0);

            finish_block();

            check_valid(h1);
            check_valid(h2);
            check_valid(h3);
            check_valid(h4);

            auto hk0 = make_key(h, k0);
            auto db_val = state_db.get_committed_value(hk0);

            REQUIRE(!!db_val);
            REQUIRE(db_val->body.nonnegative_int64() == 75);
        }

        SECTION("without set some dropped")
        {
            auto h1 = make_add_tx(k0, -50);
            auto h2 = make_add_tx(k0, -60, false);
            auto h3 = make_add_tx(k0, 5);
            auto h4 = make_add_tx(k0, 0);

            finish_block();

            check_valid(h1);
            check_invalid(h2);
            check_valid(h3);
            check_valid(h4);

            auto hk0 = make_key(h, k0);
            auto db_val = state_db.get_committed_value(hk0);

            REQUIRE(!!db_val);
            REQUIRE(db_val->body.nonnegative_int64() == 55);
        }
    }
}

TEST_CASE("raw mem storage write", "[storage]")
{
    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;
    auto& state_db = scs_data_structures.state_db;

    auto c = load_wasm_from_file("cpp_contracts/test_raw_memory.wasm");

    auto h = hash_xdr(*c);

    test::deploy_and_commit_contractdb(script_db, h, c);

    ThreadlocalContextStore::make_ctx(scs_data_structures);
    test::DeferredContextClear defer;

    auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

    InvariantKey k0 = hash_xdr<uint64_t>(0);

    std::unique_ptr<BlockContext> block_context
        = std::make_unique<BlockContext>(0);

    struct calldata_0
    {
        InvariantKey key;
    };
    struct calldata_1
    {
        InvariantKey key;
        uint64_t value;
    };

    auto make_transaction
        = [&](TransactionInvocation const& invocation)
        -> std::pair<Hash, SignedTransaction> {
        Transaction tx = Transaction(
            invocation, UINT64_MAX, 1, xdr::xvector<Contract>());
        SignedTransaction stx;
        stx.tx = tx;
        auto hash = hash_xdr(stx);
        // auto hash = tx_block->insert_tx(tx);
        return { hash, stx };
    };

    auto finish_block = [&]() {
        // phase_merge_delta_batches(*delta_batch);
        // phase_filter_deltas(scs_data_structures, *delta_batch, *tx_block);
        // phase_compute_state_updates(*delta_batch, *tx_block);
        phase_finish_block(scs_data_structures, *block_context);
    };

    auto start_new_block = [&] (uint64_t block_number) 
    {
        block_context = std::make_unique<BlockContext>(block_number);
    };

    auto make_key
        = [](Address const& addr, InvariantKey const& key) -> AddressAndKey {
        AddressAndKey out;
        std::memcpy(out.data(), addr.data(), sizeof(Address));

        std::memcpy(
            out.data() + sizeof(Address), key.data(), sizeof(InvariantKey));
        return out;
    };

    auto exec_success = [&](const Hash& tx_hash, const SignedTransaction& tx) {
        REQUIRE(exec_ctx.execute(tx_hash, tx, *block_context)
                == TransactionStatus::SUCCESS);
    };

    auto exec_fail = [&](const Hash& tx_hash, const SignedTransaction& tx) {
        REQUIRE(exec_ctx.execute(tx_hash, tx, *block_context)
                != TransactionStatus::SUCCESS);
    };

    auto require_valid = [&](const Hash& tx_hash) {
        REQUIRE(block_context->tx_set.contains_tx(tx_hash));
    };
    auto require_invalid = [&](const Hash& tx_hash) {
        REQUIRE(!block_context->tx_set.contains_tx(tx_hash));
    };

    SECTION("write to empty slot")
    {
        calldata_1 data{ .key = k0, .value = 0xAABBCCDDEEFF0011 };

        TransactionInvocation invocation(h, 1, make_calldata(data));

        auto [tx_hash, tx] = make_transaction(invocation);

        exec_success(tx_hash, tx);

        finish_block();

        require_valid(tx_hash);
        // REQUIRE(
        //	tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));

        auto hk0 = make_key(h, k0);

        auto db_val = state_db.get_committed_value(hk0);

        REQUIRE(!!db_val);

        REQUIRE(db_val->body.raw_memory_storage().data
                == xdr::opaque_vec<RAW_MEMORY_MAX_LEN>{
                    0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA });

        block_context.reset(new BlockContext(1));

        SECTION("read key from prev block")
        {
            calldata_0 data{ .key = k0 };

            TransactionInvocation invocation(h, 0, make_calldata(data));

            auto [tx_hash, tx] = make_transaction(invocation);

            exec_success(tx_hash, tx);

            auto const& logs = exec_ctx.get_logs();
            REQUIRE(logs.size() == 1);
            REQUIRE(logs[0]
                    == std::vector<uint8_t>{
                        0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA });
        }
        SECTION("overwrite key")
        {
            calldata_1 data{ .key = k0, .value = 0xA0B0C0D0E0F00010 };

            TransactionInvocation invocation(h, 1, make_calldata(data));

            auto [tx_hash, tx] = make_transaction(invocation);

            exec_success(tx_hash, tx);

            finish_block();

            require_valid(tx_hash);
            // REQUIRE(
            //	tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));

            auto hk0 = make_key(h, k0);

            auto db_val = state_db.get_committed_value(hk0);

            REQUIRE(!!db_val);

            REQUIRE(db_val->body.raw_memory_storage().data
                    == xdr::opaque_vec<RAW_MEMORY_MAX_LEN>{
                        0x10, 0x00, 0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0 });
        }

        /*
                        SECTION("delete_first key solo")
                        {
                                calldata_0 data {
                                        .key = k0
                                };

                                TransactionInvocation invocation (
                                        h,
                                        4,
                                        test::make_calldata(data)
                                );

                                auto [tx_hash, tx] = make_transaction(a0,
           invocation);

                                exec_success(tx_hash, tx);

                                finish_block();

                                REQUIRE(
                                        tx_block->is_valid(TransactionFailurePoint::FINAL,
           tx_hash));

                                auto hk0 = make_key(h, k0);

                                auto db_val = state_db.get_committed_value(hk0);

                                REQUIRE(!db_val);
                        } */

        SECTION("delete_last key solo")
        {
            calldata_0 data{ .key = k0 };

            TransactionInvocation invocation(h, 5, make_calldata(data));

            auto [tx_hash, tx] = make_transaction(invocation);

            exec_success(tx_hash, tx);

            finish_block();

            require_valid(tx_hash);

            auto hk0 = make_key(h, k0);

            auto db_val = state_db.get_committed_value(hk0);

            REQUIRE(!db_val);
        }

        SECTION("delete_last + write ok")
        {
            calldata_0 data{ .key = k0 };

            // delete_last on k0
            TransactionInvocation invocation(h, 5, make_calldata(data));

            auto [tx_hash, tx] = make_transaction(invocation);

            exec_success(tx_hash, tx);

            calldata_1 data2{ .key = k0, .value = 0xA0B0C0D0E0F00010 };

            // write on k0
            TransactionInvocation invocation2(h, 1, make_calldata(data2));

            auto [tx_hash2, tx2] = make_transaction(invocation2);

            exec_success(tx_hash2, tx2);

            finish_block();

            require_valid(tx_hash);
            require_valid(tx_hash2);
            // REQUIRE(
            //	tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));
            // REQUIRE(
            //		tx_block->is_valid(TransactionFailurePoint::FINAL,
            //tx_hash2));

            auto hk0 = make_key(h, k0);

            auto db_val = state_db.get_committed_value(hk0);

            REQUIRE(!db_val);
        }

        /*
                        SECTION("delete_first + write bad")
                        {
                                calldata_0 data {
                                        .key = k0
                                };

                                // delete_first on k0
                                TransactionInvocation invocation (
                                        h,
                                        4,
                                        test::make_calldata(data)
                                );

                                auto [tx_hash, tx] = make_transaction(a0,
           invocation);

                                exec_success(tx_hash, tx);

                                // write on k0
                                calldata_1 data2 {
                                        .key = k0,
                                        .value = 0xA0B0C0D0E0F00010
                                };

                                TransactionInvocation invocation2 (
                                        h,
                                        1,
                                        test::make_calldata(data2)
                                );

                                auto [tx_hash2, tx2] = make_transaction(a0,
           invocation2);

                                exec_ctx.reset();

                                exec_success(tx_hash2, tx2);

                                finish_block();

                                REQUIRE(
                                        !tx_block->is_valid(TransactionFailurePoint::FINAL,
           tx_hash)); REQUIRE(
                                        !tx_block->is_valid(TransactionFailurePoint::FINAL,
           tx_hash2));

                                auto hk0 = make_key(h, k0);

                                auto db_val = state_db.get_committed_value(hk0);

                                REQUIRE(!!db_val);
                                // require original value
                                REQUIRE(db_val->raw_memory_storage().data ==
           xdr::opaque_vec<RAW_MEMORY_MAX_LEN>{0x11, 0x00, 0xFF, 0xEE, 0xDD,
           0xCC, 0xBB, 0xAA});

                        } */
    }

    SECTION("get from empty slot")
    {
        calldata_0 data{ .key = k0 };

        TransactionInvocation invocation(h, 0, make_calldata(data));

        auto [tx_hash, tx] = make_transaction(invocation);

        exec_fail(tx_hash, tx);
    }

    SECTION("double write one tx ok")
    {
        calldata_0 data{ .key = k0 };

        TransactionInvocation invocation(h, 2, make_calldata(data));

        auto [tx_hash, tx] = make_transaction(invocation);

        exec_success(tx_hash, tx);
        {
            auto const& logs = exec_ctx.get_logs();
            REQUIRE(logs.size() == 2);
            REQUIRE(logs[0]
                    == std::vector<uint8_t>{
                        0x34, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
            REQUIRE(logs[1]
                == std::vector<uint8_t>{
                        0xBB, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
        }

        finish_block();

        require_valid(tx_hash);

        auto hk0 = make_key(h, k0);

        auto db_val = state_db.get_committed_value(hk0);

        REQUIRE(!!db_val);

        REQUIRE(db_val->body.raw_memory_storage().data
                == xdr::opaque_vec<RAW_MEMORY_MAX_LEN>{
                    0xBB, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
    }

    SECTION("read own writes")
    {
        calldata_0 data{ .key = k0 };

        TransactionInvocation invocation(h, 3, make_calldata(data));

        auto [tx_hash, tx] = make_transaction(invocation);

        exec_success(tx_hash, tx);

        {
            auto const& logs = exec_ctx.get_logs();
            REQUIRE(logs.size() == 1);
            REQUIRE(logs[0]
                    == std::vector<uint8_t>{
                        0x34, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
        }

        finish_block();

        require_valid(tx_hash);

        auto hk0 = make_key(h, k0);

        auto db_val = state_db.get_committed_value(hk0);

        REQUIRE(!!db_val);

        REQUIRE(db_val->body.raw_memory_storage().data
                == xdr::opaque_vec<RAW_MEMORY_MAX_LEN>{
                    0x34, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
    }
    SECTION("write after delete bad")
    {
        auto hk0 = make_key(h, k0);
        {
            calldata_1 data{ .key = k0, .value = 0xAABBCCDDEEFF0011 };

            TransactionInvocation invocation(h, 1, make_calldata(data));

            auto [tx_hash, tx] = make_transaction(invocation);

            exec_success(tx_hash, tx);

            finish_block();

            require_valid(tx_hash);
            REQUIRE(!!state_db.get_committed_value(hk0));

            start_new_block(1);
        }


        calldata_0 data{ .key = k0 };

        TransactionInvocation invocation(h, 6, make_calldata(data));

        auto [tx_hash, tx] = make_transaction(invocation);

        exec_fail(tx_hash, tx);

        finish_block();

        require_invalid(tx_hash);

        auto db_val = state_db.get_committed_value(hk0);

        REQUIRE(!!db_val);
    }
}
