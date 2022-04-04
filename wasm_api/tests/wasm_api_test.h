#include <cxxtest/TestSuite.h>

#include "debug/debug_macros.h"

#include "transaction_context/execution_context.h"

#include "wasm_api/wasm_api.h"

#include "xdr/transaction.h"

#include "test_utils/load_wasm.h"
#include "test_utils/utils.h"

using namespace scs;
using namespace scs::test;

class WasmApiTestSuite : public CxxTest::TestSuite {

public:

	void test_methodname()
	{
		TEST_START();
		MethodInvocation invocation
		{
			.addr = Address{},
			.method_name=0xABCDEF01,
			.calldata = {}
		};

		std::string expect = "pub01EFCDAB";

		auto res = invocation.get_invocable_methodname();

		TS_ASSERT_EQUALS(res, expect);
	}

	void test_methodname_zeroes()
	{
		TEST_START();
		MethodInvocation invocation
		{
			.addr = Address{},
			.method_name=0,
			.calldata = {}
		};

		std::string expect = "pub00000000";

		auto res = invocation.get_invocable_methodname();

		TS_ASSERT_EQUALS(res, expect);
	}

	void test_execute_simple()
	{
		TEST_START();
		ContractDB db;

		std::shared_ptr<Contract> c = std::make_shared<Contract>(load_wasm_from_file("wasm_api/tests/wat/test_call_simple.wasm"));

		Address addr = address_from_uint64(0);

		TS_ASSERT(db.register_contract(addr, c));

		std::unique_ptr<WasmContext> p = std::unique_ptr<WasmContext>(new Wasm3_WasmContext(db));

		ThreadlocalExecutionContext::make_ctx(std::move(p));

		auto& exec_ctx = ThreadlocalExecutionContext::get_ctx();

		MethodInvocation invocation {
			.addr = addr,
			.method_name = 1,
			.calldata = {}
		};

		TS_ASSERT_EQUALS(
			TransactionStatus::SUCCESS,
			exec_ctx.execute(invocation, UINT64_MAX));
	}

	void test_log_zeroed_array()
	{
		TEST_START();
		ContractDB db;

		std::shared_ptr<Contract> c = std::make_shared<Contract>(load_wasm_from_file("wasm_api/tests/wat/test_log.wasm"));

		Address addr = address_from_uint64(0);

		TS_ASSERT(db.register_contract(addr, c));

		std::unique_ptr<WasmContext> p = std::unique_ptr<WasmContext>(new Wasm3_WasmContext(db));

		ThreadlocalExecutionContext::make_ctx(std::move(p));

		auto& exec_ctx = ThreadlocalExecutionContext::get_ctx();

		MethodInvocation invocation {
			.addr = addr,
			.method_name = 0, //call_log
			.calldata = {}
		};

		TS_ASSERT_EQUALS(
			TransactionStatus::SUCCESS,
			exec_ctx.execute(invocation, UINT64_MAX));

		auto const& logs = exec_ctx.get_logs();

		TS_ASSERT_EQUALS(logs.size(), 1);
		
		//avoids a test segfault
		if (logs.size() > 0)
		{
			TS_ASSERT_EQUALS(logs[0].size(), 8);
			TS_ASSERT_EQUALS(logs[0], std::vector<uint8_t>({0, 0, 0, 0, 0, 0, 0, 0}));
		}
	}

	void test_log_calldata()
	{
		ContractDB db;

		std::shared_ptr<Contract> c = std::make_shared<Contract>(load_wasm_from_file("wasm_api/tests/wat/test_log.wasm"));

		Address addr = address_from_uint64(0);

		TS_ASSERT(db.register_contract(addr, c));

		std::unique_ptr<WasmContext> p = std::unique_ptr<WasmContext>(new Wasm3_WasmContext(db));

		ThreadlocalExecutionContext::make_ctx(std::move(p));

		auto& exec_ctx = ThreadlocalExecutionContext::get_ctx();

		std::vector<uint8_t> calldata = {0, 1, 2, 3, 4, 5, 6, 7};

		MethodInvocation invocation {
			.addr = addr,
			.method_name = 0, // call_log
			.calldata = calldata
		};

		TS_ASSERT_EQUALS(
			TransactionStatus::SUCCESS,
			exec_ctx.execute(invocation, UINT64_MAX));

		auto const& logs = exec_ctx.get_logs();

		TS_ASSERT_EQUALS(logs.size(), 1);
		
		//avoids a test segfault
		if (logs.size() > 0)
		{
			TS_ASSERT_EQUALS(logs[0].size(), 8);
			TS_ASSERT_EQUALS(logs[0], calldata);
		}
	}

	void test_cross_call()
	{
		ContractDB db;

		std::shared_ptr<Contract> c = std::make_shared<Contract>(load_wasm_from_file("wasm_api/tests/wat/test_cross_call_1.wasm"));
		Address addr0 = address_from_uint64(0);

		TS_ASSERT(db.register_contract(addr0, c));

		c = std::make_shared<Contract>(load_wasm_from_file("wasm_api/tests/wat/test_cross_call_2.wasm"));
		Address addr1 = address_from_uint64(1);

		TS_ASSERT(db.register_contract(addr1, c));

		std::unique_ptr<WasmContext> p = std::unique_ptr<WasmContext>(new Wasm3_WasmContext(db));
		ThreadlocalExecutionContext::make_ctx(std::move(p));
		auto& exec_ctx = ThreadlocalExecutionContext::get_ctx();

		MethodInvocation invocation {
			.addr = addr0,
			.method_name = 0, // call_log
			.calldata = {addr1.begin(), addr1.end()}
		};

		TS_ASSERT_EQUALS(
			TransactionStatus::SUCCESS,
			exec_ctx.execute(invocation, UINT64_MAX));
		
		auto const& logs = exec_ctx.get_logs();

		TS_ASSERT_EQUALS(logs.size(), 1);
		
		//avoids a test segfault
		if (logs.size() >= 1)
		{
			TS_ASSERT_EQUALS(logs[0].size(), 4);
			TS_ASSERT_EQUALS(logs[0], std::vector<uint8_t>({255, 0, 0, 0}));
		}
	}

	void test_rustsdk_log()
	{
		ContractDB db;

		std::shared_ptr<Contract> c = std::make_shared<Contract>(load_wasm_from_file("contracts/built_wasms/test_log.wasm"));
		Address addr0 = address_from_uint64(0);

		TS_ASSERT(db.register_contract(addr0, c));

		std::unique_ptr<WasmContext> p = std::unique_ptr<WasmContext>(new Wasm3_WasmContext(db));
		ThreadlocalExecutionContext::make_ctx(std::move(p));
		auto& exec_ctx = ThreadlocalExecutionContext::get_ctx();

		MethodInvocation invocation
		{
			.addr = addr0,
			.method_name = test::method_name_from_human_readable("test_log"),
			.calldata = {}
		};

		TS_ASSERT_EQUALS(
			TransactionStatus::SUCCESS,
			exec_ctx.execute(invocation, UINT64_MAX));
		
		auto const& logs = exec_ctx.get_logs();

		TS_ASSERT_EQUALS(logs.size(), 1);

		if (logs.size() >= 1)
		{
			TS_ASSERT_EQUALS(logs[0].size(), 4);
			TS_ASSERT_EQUALS(logs[0], std::vector<uint8_t>({5, 0, 0, 0}));
		}
	}
};
