#include <cxxtest/TestSuite.h>

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
};
