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

#include "debug/debug_utils.h"

#include "threadlocal/threadlocal_context.h"

#include <wasm_api/wasm_api.h>

using namespace scs;

class HashsetTests : public ::testing::Test {

 protected:
  void SetUp() override {
    auto c = load_wasm_from_file("cpp_contracts/test_hashset_manipulation.wasm");
    deploy_addr = hash_xdr(*c);

    test::deploy_and_commit_contractdb(scs_data_structures.contract_db, deploy_addr, c);
  }

  test::DeferredContextClear defer;

  Address deploy_addr;

  GlobalContext scs_data_structures = GlobalContext(wasm_api::SupportedWasmEngine::WASM3);
  std::unique_ptr<BlockContext> block_context = std::make_unique<BlockContext>(0);

  ExecutionContext<TxContext> exec_ctx = ExecutionContext<TxContext>(scs_data_structures.engine);

  void exec_success(const Hash& tx_hash, const SignedTransaction& tx) {
    EXPECT_EQ(exec_ctx.execute(tx_hash, tx, scs_data_structures, *block_context),
                TransactionStatus::SUCCESS);
  }

  void exec_fail(const Hash& tx_hash, const SignedTransaction& tx) {
    EXPECT_NE(exec_ctx.execute(tx_hash, tx, scs_data_structures, *block_context),
                TransactionStatus::SUCCESS);
  }

  void check_valid(const Hash& tx_hash) {
    EXPECT_TRUE(block_context->tx_set.contains_tx(tx_hash));
  };

  void finish_block() {
    phase_finish_block(scs_data_structures, *block_context);
  };

  void advance_block()
  {
    block_context = std::make_unique<BlockContext>(block_context -> block_number + 1);
  };

  Hash make_tx(uint32_t round, bool success = true) {

    const uint64_t gas_bid = 1;

    TransactionInvocation invocation(deploy_addr, round, make_calldata());

    Transaction tx = Transaction(
        invocation, UINT64_MAX, gas_bid, xdr::xvector<Contract>());
    SignedTransaction stx;
    stx.tx = tx;

    auto hash = hash_xdr(stx);

    if (success)
    {
        exec_success(hash, stx);
    }
    else
    {
        exec_fail(hash, stx);
    }

    return hash;
  }
};

TEST_F(HashsetTests, GoodManipulation)
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

// prevent the degenerate case of contract calling clear(),
// and then inserting something that would be cleared -- by common sense,
// the contract should see this write, but then this write will never be materialized after
// the block.  This would just be weird.
TEST_F(HashsetTests, InsertAfterClear)
{
    make_tx(1, false);
}


