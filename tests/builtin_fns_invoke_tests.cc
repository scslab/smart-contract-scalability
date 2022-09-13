#include <catch2/catch_test_macros.hpp>

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/global_context.h"
#include "transaction_context/execution_context.h"

#include "crypto/hash.h"
#include "test_utils/load_wasm.h"
#include "test_utils/make_calldata.h"

#include "state_db/modified_keys_list.h"

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

	Address sender;

	Hash h_val = hash_xdr<uint64>(0);
	std::memcpy(sender.data(), h_val.data(), h_val.size());

	BlockContext block_context;

	auto exec_success = [&] (const Hash& tx_hash, const Transaction& tx)
	{
		REQUIRE(
			exec_ctx.execute(tx_hash, tx, block_context)
			== TransactionStatus::SUCCESS);
	};

	auto exec_fail = [&] (const Hash& tx_hash, const Transaction& tx)
	{
		REQUIRE(
			exec_ctx.execute(tx_hash, tx, block_context)
			!= TransactionStatus::SUCCESS);
	};

	auto make_tx = [&] (TransactionInvocation const& invocation) -> std::pair<Hash, Transaction>
	{
		Transaction tx(sender, invocation, UINT64_MAX, 1);

		//auto h = txs.insert_tx(tx);
		return {hash_xdr(tx), tx};
	};

	SECTION("msg sender self")
	{
		TransactionInvocation invocation(
			h1,
			3,
			xdr::opaque_vec<>()
		);

		auto [h, tx] = make_tx(invocation);
		exec_success(h, tx);

		auto const& logs = exec_ctx.get_logs();

		REQUIRE(logs.size() == 1);

		if (logs.size() >= 1)
		{
			REQUIRE(logs[0].size() == 32);
			REQUIRE(memcmp(logs[0].data(), sender.data(), 32) == 0);
		}
	}

	SECTION("msg sender other")
	{
		struct calldata_t {
			Address callee;
			uint32_t method;
		};

		calldata_t data {
			.callee = h1,
			.method = 3
		};

		TransactionInvocation invocation (
			h2,
			0,
			test::make_calldata(data)
		);

		auto [h, tx] = make_tx(invocation);
		exec_success(h, tx);

		auto const& logs = exec_ctx.get_logs();

		REQUIRE(logs.size() == 1);

		if (logs.size() >= 1)
		{
			REQUIRE(logs[0].size() == 32);
			REQUIRE(memcmp(logs[0].data(), h2.data(), 32) == 0);
		}
	}

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

		auto [h, tx] = make_tx(invocation);
		exec_success(h, tx);

		auto const& logs = exec_ctx.get_logs();

		REQUIRE(logs.size() == 1);

		if (logs.size() >= 1)
		{
			REQUIRE(logs[0].size() == 1);
			REQUIRE(logs[0] == std::vector<uint8_t>{0xFF});
		}
	}

	SECTION("invoke self with reentrance guard")
	{
		struct calldata_t {
			Address callee;
			uint32_t method;
		};

		calldata_t data {
			.callee = h2,
			.method = 2
		};

		TransactionInvocation invocation (
			h2,
			0,
			test::make_calldata(data)
		);

		auto [h, tx] = make_tx(invocation);
		exec_fail(h, tx);

	}

	SECTION("invoke other")
	{
		struct calldata_t {
			Address callee;
			uint32_t method;
		};

		calldata_t data {
			.callee = h1,
			.method = 0
		};

		TransactionInvocation invocation (
			h2,
			0,
			test::make_calldata(data)
		);

		auto [h, tx] = make_tx(invocation);
		exec_success(h, tx);

		auto const& logs = exec_ctx.get_logs();

		REQUIRE(logs.size() == 1);
	}

	SECTION("invoke insufficient calldata")
	{
		TransactionInvocation invocation (
			h1,
			1,
			xdr::opaque_vec<>()
		);

		auto [h, tx] = make_tx(invocation);
		exec_fail(h, tx);
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

		auto [h, tx] = make_tx(invocation);
		exec_fail(h, tx);
	}
}

} /* scs */

