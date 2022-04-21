#include <cxxtest/TestSuite.h>

#include "debug/debug_macros.h"

#include "transaction_context/execution_context.h"
#include "transaction_context/threadlocal_context.h"

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
		MethodInvocation invocation(
			Address{},
			0xABCDEF01,
			std::vector<uint8_t>()
		);

		std::string expect = "pub01EFCDAB";

		auto res = invocation.get_invocable_methodname();

		TS_ASSERT_EQUALS(res, expect);
	}

	void test_methodname_zeroes()
	{
		TEST_START();
		MethodInvocation invocation(
			Address{},
			0,
			std::vector<uint8_t>()
		);

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

		ThreadlocalContextStore::make_ctx(std::move(p));

		auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

		TransactionInvocation invocation (
			addr,
			1,
			xdr::opaque_vec<>()
		);

		TS_ASSERT_EQUALS(
			TransactionStatus::SUCCESS,
			exec_ctx.execute(Transaction(invocation, UINT64_MAX, 1)));
	}

	void test_log_zeroed_array()
	{
		TEST_START();
		ContractDB db;

		std::shared_ptr<Contract> c = std::make_shared<Contract>(load_wasm_from_file("wasm_api/tests/wat/test_log.wasm"));

		Address addr = address_from_uint64(0);

		TS_ASSERT(db.register_contract(addr, c));

		std::unique_ptr<WasmContext> p = std::unique_ptr<WasmContext>(new Wasm3_WasmContext(db));

		ThreadlocalContextStore::make_ctx(std::move(p));

		auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

		TransactionInvocation invocation (
			addr,
			0, //call_log
			xdr::opaque_vec<>({0, 1, 0, 2, 0, 3, 0, 4})
		);

		Transaction tx(
			invocation,
			UINT64_MAX,
			1);

		TS_ASSERT_EQUALS(
			TransactionStatus::SUCCESS,
			exec_ctx.execute(tx));

		auto const& logs = exec_ctx.get_logs();

		TS_ASSERT_EQUALS(logs.size(), 1);
		
		//avoids a test segfault
		if (logs.size() > 0)
		{
			TS_ASSERT_EQUALS(logs[0].size(), 8);
			TS_ASSERT_EQUALS(logs[0], std::vector<uint8_t>({0, 1, 0, 2, 0, 3, 0, 4}));
		}
	}

	void test_log_calldata()
	{
		ContractDB db;

		std::shared_ptr<Contract> c = std::make_shared<Contract>(load_wasm_from_file("wasm_api/tests/wat/test_log.wasm"));

		Address addr = address_from_uint64(0);

		TS_ASSERT(db.register_contract(addr, c));

		std::unique_ptr<WasmContext> p = std::unique_ptr<WasmContext>(new Wasm3_WasmContext(db));

		ThreadlocalContextStore::make_ctx(std::move(p));

		auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

		xdr::opaque_vec<>calldata = {0, 1, 2, 3, 4, 5, 6, 7};

		TransactionInvocation invocation (
			addr,
			0, // call_log
			calldata
		);

		TS_ASSERT_EQUALS(
			TransactionStatus::SUCCESS,
			exec_ctx.execute(Transaction(invocation, UINT64_MAX, 1)));

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
		ThreadlocalContextStore::make_ctx(std::move(p));
		auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

		TransactionInvocation invocation (
			addr0,
			0, // call_log
			xdr::opaque_vec<>{addr1.begin(), addr1.end()}
		);

		TS_ASSERT_EQUALS(
			TransactionStatus::SUCCESS,
			exec_ctx.execute(Transaction(invocation, UINT64_MAX, 1)));
		
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
		ThreadlocalContextStore::make_ctx(std::move(p));
		auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

		TransactionInvocation invocation (
			addr0,
			test::method_name_from_human_readable("test_log"),
			xdr::opaque_vec<>{}
		);

		TS_ASSERT_EQUALS(
			TransactionStatus::SUCCESS,
			exec_ctx.execute(Transaction(invocation, UINT64_MAX, 1)));
		
		auto const& logs = exec_ctx.get_logs();

		TS_ASSERT_EQUALS(logs.size(), 1);

		if (logs.size() >= 1)
		{
			TS_ASSERT_EQUALS(logs[0].size(), 4);
			TS_ASSERT_EQUALS(logs[0], std::vector<uint8_t>({48, 0, 0, 0}));
		}
	}

	void test_rustsdk_log_fancy()
	{
		ContractDB db;

		std::shared_ptr<Contract> c = std::make_shared<Contract>(load_wasm_from_file("contracts/built_wasms/test_log.wasm"));
		Address addr0 = address_from_uint64(0);

		TS_ASSERT(db.register_contract(addr0, c));

		std::unique_ptr<WasmContext> p = std::unique_ptr<WasmContext>(new Wasm3_WasmContext(db));
		ThreadlocalContextStore::make_ctx(std::move(p));
		auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

		uint32_t val1 = 5;

		uint64_t val2 = 0xAABBCCDDEEFF0011;

		struct calldata_format{
			Address addr;
			uint32_t v1;
			uint64_t v2;
		} to_be_serialized;

		to_be_serialized.addr = addr0;
		to_be_serialized.v1 = val1;
		to_be_serialized.v2 = val2;

		uint8_t* ptr = reinterpret_cast<uint8_t*>(&to_be_serialized);

		TransactionInvocation invocation (
			addr0,
			test::method_name_from_human_readable("try_fancy_call"),
			xdr::opaque_vec<>{ptr, ptr + sizeof(to_be_serialized)}
		);

		TS_ASSERT_EQUALS(
			TransactionStatus::SUCCESS,
			exec_ctx.execute(Transaction(invocation, UINT64_MAX, 1)));
		
		auto const& logs = exec_ctx.get_logs();

		TS_ASSERT_EQUALS(logs.size(), 3);

		if (logs.size() >= 3)
		{
			TS_ASSERT_EQUALS(logs[0].size(), 32);

			TS_ASSERT_EQUALS(logs[1], std::vector<uint8_t>({5, 0, 0, 0}));

			TS_ASSERT_EQUALS(logs[2].size(), 8);
			if (logs[2].size() >= 8)
			{
				uint64_t val2_res = utils::read_unsigned_little_endian<uint64_t>(logs[2].data());
				TS_ASSERT_EQUALS(val2_res, val2);
			}
		}
	}
};
