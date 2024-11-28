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

#include "crypto/hash.h"
#include "test_utils/deploy_and_commit_contractdb.h"
#include "utils/load_wasm.h"
#include "utils/make_calldata.h"

#include "debug/debug_utils.h"

#include "threadlocal/threadlocal_context.h"

#include <wasm_api/wasm_api.h>

using namespace scs;

class GasTests : public ::testing::Test {

 protected:
  void SetUp() override {
    auto c = load_wasm_from_file("cpp_contracts/test_gas_limit.wasm");
    deploy_addr = hash_xdr(*c);

    test::deploy_and_commit_contractdb(scs_data_structures.contract_db, deploy_addr, c);
  }

  test::DeferredContextClear defer;

  Address deploy_addr;

  GlobalContext scs_data_structures = GlobalContext(wasm_api::SupportedWasmEngine::WASM3);
  BlockContext block_context = BlockContext(0);

  ExecutionContext<TxContext> exec_ctx = ExecutionContext<TxContext>(scs_data_structures.engine);


  void exec_success(const Hash& tx_hash, const SignedTransaction& tx) {
    EXPECT_EQ(exec_ctx.execute(tx_hash, tx, scs_data_structures, block_context),
                TransactionStatus::SUCCESS);
  }

  void exec_fail(const Hash& tx_hash, const SignedTransaction& tx) {
    EXPECT_NE(exec_ctx.execute(tx_hash, tx, scs_data_structures, block_context),
                TransactionStatus::SUCCESS);
  }

  Hash
  make_spin_tx(uint64_t duration,
                uint64_t gas_limit,
                bool success = true) {
        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(deploy_addr, 0, make_calldata(duration));

        Transaction tx = Transaction(
            invocation, gas_limit, gas_bid, xdr::xvector<Contract>());
        SignedTransaction stx;
        stx.tx = tx;

        auto hash = hash_xdr(stx);

        if (success) {
            EXPECT_EQ(exec_ctx.execute(hash, stx, scs_data_structures, block_context)
                    , TransactionStatus::SUCCESS);
        } else {
            EXPECT_NE(exec_ctx.execute(hash, stx, scs_data_structures, block_context)
                    , TransactionStatus::SUCCESS);
        }

        return hash;
    }

    Hash make_loop_tx(uint64_t gas_limit) {
        const uint64_t gas_bid = 1;

        TransactionInvocation invocation(deploy_addr, 1, make_calldata());

        Transaction tx = Transaction(
            invocation, gas_limit, gas_bid, xdr::xvector<Contract>());
        SignedTransaction stx;
        stx.tx = tx;

        auto hash = hash_xdr(stx);

        EXPECT_NE(exec_ctx.execute(hash, stx, scs_data_structures, block_context)
                , TransactionStatus::SUCCESS);

        return hash;
    };
};

TEST_F(GasTests, SpinShort) {
    make_spin_tx(3, 10000);
}

TEST_F(GasTests, SpinLong) {
    make_spin_tx(10000, 10000, false);
}


TEST_F(GasTests, LowGas) {
    make_spin_tx(0, 1, false);
}


TEST_F(GasTests, LoopShort) {
    make_loop_tx(1);
}


TEST_F(GasTests, LoopLong) {
    make_loop_tx(10000);
}


