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
			.method_name = "call_with_calldata",
			.calldata = {}
		};

		TS_ASSERT_EQUALS(
			TransactionStatus::SUCCESS,
			exec_ctx.execute(invocation, UINT64_MAX));



	}
};
