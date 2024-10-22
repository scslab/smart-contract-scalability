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

#include <gtest/gtest.h>

#include "transaction_context/global_context.h"
#include "transaction_context/execution_context.h"

#include "phase/phases.h"

#include "crypto/hash.h"
#include "test_utils/deploy_and_commit_contractdb.h"
#include "utils/load_wasm.h"
#include "utils/make_calldata.h"

#include "vm/genesis.h"

#include "debug/debug_utils.h"

#include "threadlocal/threadlocal_context.h"

#include "xdr/types.h"

using namespace scs;

class SdkTests : public ::testing::Test {

 protected:
  void SetUp() override {
    auto c = load_wasm_from_file("cpp_contracts/test_sdk.wasm");
    deploy_addr = hash_xdr(*c);

    test::deploy_and_commit_contractdb(scs_data_structures.contract_db, deploy_addr, c);

    sem_addr = make_addrkey(deploy_addr, make_address(0, 1, 0, 0));
    sem2_addr = make_addrkey(deploy_addr, make_address(1, 1, 0, 0));
    transient_addr = make_addrkey(deploy_addr, make_address(2, 1, 0, 0));
  }

  test::DeferredContextClear defer;

  Address deploy_addr;

  GlobalContext scs_data_structures;
  std::unique_ptr<BlockContext> block_context = std::make_unique<BlockContext>(0);

  ExecutionContext<TxContext> exec_ctx;

  Hash make_replay_tx(uint64_t nonce, uint64_t expiry_time, bool success = true) {
        struct calldata_0
        {
            uint64_t expiry_time;
            uint64_t nonce; // ignored in the contract
        };

        calldata_0 data
        {
            .expiry_time = expiry_time,
            .nonce = nonce
        };

        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(deploy_addr, 0, make_calldata(data));

        Transaction tx = Transaction(
            invocation, UINT64_MAX, gas_bid, xdr::xvector<Contract>());
        SignedTransaction stx;
        stx.tx = tx;

        auto hash = hash_xdr(stx);

        if (success) {
            EXPECT_TRUE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
                    == TransactionStatus::SUCCESS);
        } else {
            EXPECT_TRUE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
                    != TransactionStatus::SUCCESS);
        }

        return hash;
    }

    void check_valid (const Hash& tx_hash, uint32_t multiplicity = 1) {
        ASSERT_EQ(block_context->tx_set.contains_tx(tx_hash), multiplicity);
    };

    void check_invalid(const Hash& tx_hash) {
        ASSERT_EQ(block_context->tx_set.contains_tx(tx_hash), 0);
    };

    void finish_block() {
        phase_finish_block(scs_data_structures, *block_context);
    };

    void advance_block ()
    {
        block_context = std::make_unique<BlockContext>(block_context->block_number + 1);
    };

    Hash make_semaphore_tx(bool success = true) {

        struct calldata_0 {};

        calldata_0 data
        {
        };

        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(deploy_addr, 1, make_calldata(data));

        Transaction tx = Transaction(
            invocation, UINT64_MAX, gas_bid, xdr::xvector<Contract>());
        SignedTransaction stx;
        stx.tx = tx;

        auto hash = hash_xdr(stx);
      
        if (success) {
            EXPECT_TRUE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
                    == TransactionStatus::SUCCESS);
        } else {
            EXPECT_TRUE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
                    != TransactionStatus::SUCCESS);
        }

        return hash;
    }

    // a semaphore that can be acquired twice (i.e. set_add (2, -1))
    Hash make_semaphore2_tx(bool success = true) {
        struct calldata_0 {};

        calldata_0 data
        {
        };

        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(deploy_addr, 2, make_calldata(data));

        Transaction tx = Transaction(
            invocation, UINT64_MAX, gas_bid, xdr::xvector<Contract>());
        
        SignedTransaction stx;
        stx.tx = tx;
        
        auto hash = hash_xdr(stx);

        if (success) {
            EXPECT_TRUE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
                    == TransactionStatus::SUCCESS);
        } else {
            EXPECT_TRUE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
                    != TransactionStatus::SUCCESS);
        }

        return hash;
    };

    Hash make_transient_semaphore_tx(bool success = true)
    {
        struct calldata_0 {};

        calldata_0 data
        {
        };


        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(deploy_addr, 3, make_calldata(data));

        Transaction tx = Transaction(
            invocation, UINT64_MAX, gas_bid, xdr::xvector<Contract>());
        
        SignedTransaction stx;
        stx.tx = tx;
        
        auto hash = hash_xdr(stx);

        if (success) {
            EXPECT_EQ(exec_ctx.execute(hash, stx, scs_data_structures, *block_context), TransactionStatus::SUCCESS);
        } else {
            EXPECT_NE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context),  TransactionStatus::SUCCESS);
        }

        return hash;
    }

    AddressAndKey make_addrkey(const Address& a, const InvariantKey& k)
    {
        AddressAndKey out;
        std::memcpy(out.data(), a.data(), a.size());
        std::memcpy(out.data() + a.size(), k.data(), k.size());
        return out;
    };

    // hardcoded in the sdk tests contract
    AddressAndKey sem_addr;
    AddressAndKey sem2_addr;
    AddressAndKey transient_addr;
};

TEST_F(SdkTests, ReplayCacheOneBlock)
{
    auto h0 = make_replay_tx(0, UINT64_MAX);
    auto h1 = make_replay_tx(1, UINT64_MAX);

    make_replay_tx(0, UINT64_MAX, false);
    make_replay_tx(1, UINT64_MAX, false);

    finish_block();

    check_valid(h0);
    check_valid(h1);
}

TEST_F(SdkTests, ReplayCacheFill)
{
    for (size_t i = 0; i < 64; i++)
    {
        make_replay_tx(i, 0);
        make_replay_tx(i, 0, false);
    }

    // fill up replay cache
    make_replay_tx(500, 0, false);

    finish_block();

    // next block can insert again until full
    advance_block();
    for (size_t i = 0; i < 64; i++)
    {
        make_replay_tx(i, 1);
        make_replay_tx(i, 1, false);
    }
    make_replay_tx(500, 1, false);
}

TEST_F(SdkTests, SemaphoreTests) 
{
 
    // One semaphore acquire in one block ok
    auto h = make_semaphore_tx();

    auto h2 = make_semaphore_tx(false);
    auto h3 = make_semaphore_tx(false);

    finish_block();
    check_valid(h, 1);
    EXPECT_EQ(h, h2);
    EXPECT_EQ(h, h3);

    advance_block();

    // can be reaquired in future blocks
    make_semaphore_tx();

    finish_block();
    advance_block();

    make_semaphore_tx();

    finish_block();
    advance_block();

    make_semaphore_tx();

    finish_block();

    ASSERT_TRUE(!!scs_data_structures.state_db.get_committed_value(sem_addr));
    ASSERT_TRUE(scs_data_structures.state_db.get_committed_value(sem_addr) -> body.nonnegative_int64() == 0);    
}

TEST_F(SdkTests, DoubleSemaphore2Ok)
{
    auto h1 = make_semaphore2_tx();
    auto h2 = make_semaphore2_tx();
    auto h3 = make_semaphore2_tx(false);
    EXPECT_EQ(h1, h2);
    EXPECT_EQ(h2, h3);

    finish_block();

    check_valid(h1, 2);
    
    ASSERT_TRUE(!!scs_data_structures.state_db.get_committed_value(sem2_addr));
    ASSERT_TRUE(scs_data_structures.state_db.get_committed_value(sem2_addr) -> body.nonnegative_int64() == 0);
}

TEST_F(SdkTests, TransientSemaphoreCleanup)
{
    make_transient_semaphore_tx();

    finish_block();

    ASSERT_TRUE(!scs_data_structures.state_db.get_committed_value(transient_addr));
}

TEST_F(SdkTests, TransientSemaphoreConflict)
{
    make_transient_semaphore_tx();
    make_transient_semaphore_tx(false);

    finish_block();

    ASSERT_TRUE(!scs_data_structures.state_db.get_committed_value(transient_addr));
}


/*

TEST_CASE("semaphore", "[sdk]")
{
    test::DeferredContextClear defer;

    GlobalContext scs_data_structures;
    auto& script_db = scs_data_structures.contract_db;

    auto c = load_wasm_from_file("cpp_contracts/test_sdk.wasm");

    auto h = hash_xdr(*c);

    test::deploy_and_commit_contractdb(script_db, h, c);

    std::unique_ptr<BlockContext> block_context
        = std::make_unique<BlockContext>(0);

    struct calldata_0
    {
    };

    ExecutionContext<TxContext> exec_ctx;

    auto make_semaphore_tx = [&](bool success = true) -> Hash {

        calldata_0 data
        {
        };

        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(h, 1, make_calldata(data));

        Transaction tx = Transaction(
            invocation, UINT64_MAX, gas_bid, xdr::xvector<Contract>());
        SignedTransaction stx;
        stx.tx = tx;

        auto hash = hash_xdr(stx);
      
        if (success) {
            REQUIRE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
                    == TransactionStatus::SUCCESS);
        } else {
            REQUIRE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
                    != TransactionStatus::SUCCESS);
        }

        return hash;
    };

    auto make_semaphore2_tx = [&](bool success = true) -> Hash {

        calldata_0 data
        {
        };


        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(h, 2, make_calldata(data));

        Transaction tx = Transaction(
            invocation, UINT64_MAX, gas_bid, xdr::xvector<Contract>());
        
        SignedTransaction stx;
        stx.tx = tx;
        
        auto hash = hash_xdr(stx);

        if (success) {
            REQUIRE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
                    == TransactionStatus::SUCCESS);
        } else {
            REQUIRE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
                    != TransactionStatus::SUCCESS);
        }

        return hash;
    };

    auto make_transient_semaphore_tx = [&](bool success = true) -> Hash
    {
        calldata_0 data
        {
        };


        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(h, 3, make_calldata(data));

        Transaction tx = Transaction(
            invocation, UINT64_MAX, gas_bid, xdr::xvector<Contract>());
        
        SignedTransaction stx;
        stx.tx = tx;
        
        auto hash = hash_xdr(stx);

        if (success) {
            REQUIRE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
                    == TransactionStatus::SUCCESS);
        } else {
            REQUIRE(exec_ctx.execute(hash, stx, scs_data_structures, *block_context)
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

    auto make_addrkey = [](const Address& a, const InvariantKey& k)
    {
        AddressAndKey out;
        std::memcpy(out.data(), a.data(), a.size());
        std::memcpy(out.data() + a.size(), k.data(), k.size());
        return out;
    };

    auto sem_addr = make_addrkey(h, make_address(0, 1, 0, 0));
    auto sem2_addr = make_addrkey(h, make_address(1, 1, 0, 0));

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

        REQUIRE(!!scs_data_structures.state_db.get_committed_value(sem_addr));
        REQUIRE(scs_data_structures.state_db.get_committed_value(sem_addr) -> body.nonnegative_int64() == 0);

    }
    SECTION("double semaphore 2 ok")
    {
    	make_semaphore2_tx();
    	make_semaphore2_tx();
    	make_semaphore2_tx(false);
        finish_block();
        
        REQUIRE(!!scs_data_structures.state_db.get_committed_value(sem2_addr));
        REQUIRE(scs_data_structures.state_db.get_committed_value(sem2_addr) -> body.nonnegative_int64() == 0);
    }

    auto transient_addr = make_addrkey(h, make_address(2, 1, 0, 0));

    SECTION("transient semaphore leaves nothing behind")
    {
        make_transient_semaphore_tx();

        finish_block();

        REQUIRE(!scs_data_structures.state_db.get_committed_value(transient_addr));
    }
    SECTION("transient semaphore conflict")
    {
        make_transient_semaphore_tx();
        make_transient_semaphore_tx(false);

        finish_block();

        REQUIRE(!scs_data_structures.state_db.get_committed_value(transient_addr));
    }
} */
