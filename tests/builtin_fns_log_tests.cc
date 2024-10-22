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

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/global_context.h"

#include "crypto/hash.h"
#include "utils/load_wasm.h"

#include "test_utils/deploy_and_commit_contractdb.h"

namespace scs {

class BuiltinLogTests : public ::testing::Test {

 protected:
  void SetUp() override {
    auto c = load_wasm_from_file("cpp_contracts/test_log.wasm");
    deploy_addr = hash_xdr(*c);

    test::deploy_and_commit_contractdb(scs_data_structures.contract_db, deploy_addr, c);
  }

  void TearDown() override {
    test::DeferredContextClear defer;
  }

  Address deploy_addr;

  GlobalContext scs_data_structures;
  BlockContext block_context = BlockContext(0);

  ExecutionContext<TxContext> exec_ctx;


  void exec_success(const Hash& tx_hash, const SignedTransaction& tx) {
    EXPECT_EQ(exec_ctx.execute(tx_hash, tx, scs_data_structures, block_context),
                TransactionStatus::SUCCESS);
  }

  void exec_fail(const Hash& tx_hash, const SignedTransaction& tx) {
    EXPECT_NE(exec_ctx.execute(tx_hash, tx, scs_data_structures, block_context),
                TransactionStatus::SUCCESS);
  }

  std::pair<Hash, SignedTransaction>
  make_tx(TransactionInvocation const& invocation) {
    Transaction tx(
        invocation, UINT64_MAX, 1, xdr::xvector<Contract>());

    SignedTransaction stx;
    stx.tx = tx;

    return { hash_xdr(stx), stx };
  }
};

TEST_F(BuiltinLogTests, TestLogHardcoded)
{
    TransactionInvocation invocation(deploy_addr, 0, xdr::opaque_vec<>());

    auto [h, tx] = make_tx(invocation);

    exec_success(h, tx);

    auto const& logs = exec_ctx.get_logs();

    ASSERT_EQ(logs.size(), 1);

    EXPECT_EQ(logs[0].size(), 8);
    EXPECT_EQ(logs[0],
             (std::vector<uint8_t>{0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA }));
}

TEST_F(BuiltinLogTests, TestLogCalldata)
{
 
    TransactionInvocation invocation(
        deploy_addr,
        1,
        xdr::opaque_vec<>{
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 });

    auto [h, tx] = make_tx(invocation);
    exec_success(h, tx);

    auto const& logs = exec_ctx.get_logs();

    ASSERT_EQ(logs.size(), 1);

    EXPECT_EQ(logs[0].size(), 8);
    EXPECT_EQ(logs[0],
            (std::vector<uint8_t>{
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 }));
    
}

TEST_F(BuiltinLogTests, TestLogTwice)
{
    test::DeferredContextClear defer;
    

    TransactionInvocation invocation(
        deploy_addr,
        2,
        xdr::opaque_vec<>{
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 });

    auto [h, tx] = make_tx(invocation);
    exec_success(h, tx);

    auto const& logs = exec_ctx.get_logs();

    ASSERT_EQ(logs.size(),2);

    EXPECT_EQ(logs[0].size(), 4);
    EXPECT_EQ(logs[0], (std::vector<uint8_t>{ 0x00, 0x01, 0x02, 0x03 }));
    EXPECT_EQ(logs[1], (std::vector<uint8_t>{ 0x04, 0x05, 0x06, 0x07 }));
}

} // namespace scs
