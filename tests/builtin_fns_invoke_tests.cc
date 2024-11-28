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
#include "test_utils/deploy_and_commit_contractdb.h"
#include "utils/load_wasm.h"
#include "utils/make_calldata.h"

#include "state_db/modified_keys_list.h"

#include <wasm_api/wasm_api.h>

using xdr::operator==;

namespace scs {

class BuiltinInvokeTests : public ::testing::Test {

 protected:
  void SetUp() override {
    auto c1 = load_wasm_from_file("cpp_contracts/test_log.wasm");
    deploy_addr1 = hash_xdr(*c1);

    test::deploy_and_commit_contractdb(scs_data_structures.contract_db, deploy_addr1, c1);

    auto c2 = load_wasm_from_file("cpp_contracts/test_redirect_call.wasm");
    deploy_addr2 = hash_xdr(*c2);

    test::deploy_and_commit_contractdb(scs_data_structures.contract_db, deploy_addr2, c2);
  }

  void TearDown() override {
    test::DeferredContextClear defer;
  }

  Address deploy_addr1;
  Address deploy_addr2;

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

  std::pair<Hash, SignedTransaction>
  make_tx(TransactionInvocation const& invocation) {
    Transaction tx(
        invocation, UINT64_MAX, 1, xdr::xvector<Contract>());

    SignedTransaction stx;
    stx.tx = tx;

    return { hash_xdr(stx), stx };
  }
};

TEST_F(BuiltinInvokeTests, LogSelfAddr)
{
    TransactionInvocation invocation(deploy_addr1, 4, xdr::opaque_vec<>());

    auto [h, tx] = make_tx(invocation);
    exec_success(h, tx);

    auto const& logs = exec_ctx.get_logs();

    ASSERT_EQ(logs.size(), 1);

    ASSERT_EQ(logs[0].size(), 32);
    EXPECT_EQ(memcmp(logs[0].data(), deploy_addr1.data(), 32), 0);
}

TEST_F(BuiltinInvokeTests, MsgSenderSelfFailsNoRedirect)
{
    TransactionInvocation invocation(deploy_addr1, 3, xdr::opaque_vec<>());

    auto [h, tx] = make_tx(invocation);
    exec_fail(h, tx);
}

TEST_F(BuiltinInvokeTests, MsgSenderOther)
{
    struct calldata_t
    {
        Address callee;
        uint32_t method;
    };

    calldata_t data{ .callee = deploy_addr1, .method = 3 };

    TransactionInvocation invocation(deploy_addr2, 0, make_calldata(data));

    auto [h, tx] = make_tx(invocation);
    exec_success(h, tx);

    auto const& logs = exec_ctx.get_logs();

    ASSERT_EQ(logs.size(), 1);

    ASSERT_EQ(logs[0].size(), 32);
    EXPECT_EQ(memcmp(logs[0].data(), deploy_addr2.data(), 32), 0);
}

TEST_F(BuiltinInvokeTests, InvokeSelf)
{
    struct calldata_t
    {
        Address callee;
        uint32_t method;
    };

    calldata_t data{ .callee = deploy_addr2, .method = 1 };

    TransactionInvocation invocation(deploy_addr2, 0, make_calldata(data));

    auto [h, tx] = make_tx(invocation);
    exec_success(h, tx);

    auto const& logs = exec_ctx.get_logs();

    ASSERT_EQ(logs.size(), 1);

    ASSERT_EQ(logs[0].size(), 1);
    EXPECT_EQ(logs[0], std::vector<uint8_t>{ 0xFF });
}

TEST_F(BuiltinInvokeTests, InvokeSelfReentranceGuard)
{
    struct calldata_t
    {
        Address callee;
        uint32_t method;
    };

    calldata_t data{ .callee = deploy_addr2, .method = 2 };

    TransactionInvocation invocation(deploy_addr2, 0, make_calldata(data));

    auto [h, tx] = make_tx(invocation);
    exec_fail(h, tx);   
}

TEST_F(BuiltinInvokeTests, InvokeOther)
{
    struct calldata_t
    {
        Address callee;
        uint32_t method;
    };

    calldata_t data{ .callee = deploy_addr1, .method = 0 };

    TransactionInvocation invocation(deploy_addr2, 0, make_calldata(data));

    auto [h, tx] = make_tx(invocation);
    exec_success(h, tx);

    auto const& logs = exec_ctx.get_logs();

    EXPECT_EQ(logs.size(), 1);
}

TEST_F(BuiltinInvokeTests, InvokeInsufficientCalldata)
{
    TransactionInvocation invocation(deploy_addr1, 1, xdr::opaque_vec<>());

    auto [h, tx] = make_tx(invocation);
    exec_fail(h, tx);
}

TEST_F(BuiltinInvokeTests, InvokeNonexistent)
{
    struct calldata_t
    {
        Address callee;
        uint32_t method;
    };

    Address empty_addr;

    calldata_t data{ .callee = empty_addr, .method = 1 };

    TransactionInvocation invocation(deploy_addr2, 0, make_calldata(data));

    auto [h, tx] = make_tx(invocation);
    exec_fail(h, tx);
}

} // namespace scs
