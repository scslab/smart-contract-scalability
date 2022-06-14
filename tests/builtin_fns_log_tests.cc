#include <catch2/catch_test_macros.hpp>

#include "transaction_context/global_context.h"
#include "transaction_context/threadlocal_context.h"
#include "transaction_context/execution_context.h"

#include "crypto/hash.h"
#include "test_utils/load_wasm.h"

#include "state_db/delta_batch.h"

namespace scs
{

TEST_CASE("test log", "[builtin]")
{
	GlobalContext scs_data_structures;
	auto& script_db = scs_data_structures.contract_db;

	auto c = test::load_wasm_from_file("cpp_contracts/test_log.wasm");
	auto h = hash_xdr(*c);

	REQUIRE(script_db.register_contract(h,std::move(c)));

	ThreadlocalContextStore::make_ctx(scs_data_structures);
	test::DeferredContextClear defer;

	auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

	Address sender;
	TxBlock txs;
	DeltaBatch delta_batch;

	auto exec_success = [&] (const Hash& tx_hash, const Transaction& tx)
	{
		REQUIRE(
			exec_ctx.execute(tx_hash, tx, txs, delta_batch)
			== TransactionStatus::SUCCESS);
	};

	auto exec_fail = [&] (const Hash& tx_hash, const Transaction& tx)
	{
		REQUIRE(
			exec_ctx.execute(tx_hash, tx, txs, delta_batch)
			!= TransactionStatus::SUCCESS);
	};


	auto make_tx = [&] (TransactionInvocation const& invocation) -> std::pair<Hash, Transaction>
	{
		Transaction tx(sender, invocation, UINT64_MAX, 1);

		auto h = txs.insert_tx(tx);
		return {h, tx};
	};

	SECTION("log hardcoded")
	{
		TransactionInvocation invocation (
			h,
			0,
			xdr::opaque_vec<>()
		);

		auto [h, tx] = make_tx(invocation);

		exec_success(h, tx);

		auto const& logs = exec_ctx.get_logs();

		REQUIRE(logs.size() == 1);

		if (logs.size() >= 1)
		{
			REQUIRE(logs[0].size() == 8);
			REQUIRE(logs[0] == std::vector<uint8_t>{0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA});
		}
	}
	SECTION("log calldata")
	{
		TransactionInvocation invocation (
			h,
			1,
			xdr::opaque_vec<>{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}
		);

		auto [h, tx] = make_tx(invocation);
		exec_success(h, tx);

		auto const& logs = exec_ctx.get_logs();

		REQUIRE(logs.size() == 1);

		if (logs.size() >= 1)
		{
			REQUIRE(logs[0].size() == 8);
			REQUIRE(logs[0] == std::vector<uint8_t>{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
		}
	}

	SECTION("log twice")
	{
		TransactionInvocation invocation (
			h,
			2,
			xdr::opaque_vec<>{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}
		);

		auto [h, tx] = make_tx(invocation);
		exec_success(h, tx);

		auto const& logs = exec_ctx.get_logs();

		REQUIRE(logs.size() == 2);

		if (logs.size() >= 2)
		{
			REQUIRE(logs[0].size() == 4);
			REQUIRE(logs[0] == std::vector<uint8_t>{0x00, 0x01, 0x02, 0x03});
			REQUIRE(logs[1] == std::vector<uint8_t>{0x04, 0x05, 0x06, 0x07});
		}
	}
}

} /* scs */
