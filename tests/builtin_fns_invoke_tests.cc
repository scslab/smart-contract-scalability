#include <catch2/catch_test_macros.hpp>

#include "transaction_context/global_context.h"
#include "transaction_context/threadlocal_context.h"
#include "transaction_context/execution_context.h"

#include "crypto/hash.h"
#include "test_utils/load_wasm.h"
#include "test_utils/make_calldata.h"

namespace scs
{

TEST_CASE("test invoke", "[builtin]")
{
	GlobalContext scs_data_structures;
	auto& script_db = scs_data_structures.contract_db;

	auto c1 = test::load_wasm_from_file("cpp_contracts/test_log.wasm");
	auto h1 = hash_xdr(*c1);

	auto c2 = test::load_wasm_from_file("cpp_contracts/test_redirect_call.wasm");
	auto h2 = hash_xdr(*c2);

	REQUIRE(script_db.register_contract(h1, std::move(c1)));
	REQUIRE(script_db.register_contract(h2, std::move(c2)));

	ThreadlocalContextStore::make_ctx(scs_data_structures);
	test::DeferredContextClear defer;

	auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

	SECTION("invoke self")
	{
		struct calldata_t {
			Address callee;
			uint32_t method;
		};

		calldata_t data {
			.callee = h2,
			.method = 1
		};

		TransactionInvocation invocation (
			h2,
			0,
			test::make_calldata(data)
		);

		std::printf("calldata: %s\n", debug::array_to_str(invocation.calldata).c_str());

		REQUIRE(
			exec_ctx.execute(Transaction(invocation, UINT64_MAX, 1))
			== TransactionStatus::SUCCESS);

		auto const& logs = exec_ctx.get_logs();

		REQUIRE(logs.size() == 1);

		if (logs.size() >= 1)
		{
			REQUIRE(logs[0].size() == 1);
			REQUIRE(logs[0] == std::vector<uint8_t>{0xFF});
		}
	}

	SECTION("invoke nonexistent")
	{
		struct calldata_t {
			Address callee;
			uint32_t method;
		};

		Address empty_addr;

		calldata_t data {
			.callee = empty_addr,
			.method = 1
		};

		TransactionInvocation invocation (
			h2,
			0,
			test::make_calldata(data)
		);

		REQUIRE(
			exec_ctx.execute(Transaction(invocation, UINT64_MAX, 1))
			!= TransactionStatus::SUCCESS);
	}
}

} /* scs */

