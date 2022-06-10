#include <catch2/catch_test_macros.hpp>

#include "transaction_context/global_context.h"

#include "phase/phases.h"

#include "crypto/hash.h"
#include "test_utils/load_wasm.h"
#include "test_utils/make_calldata.h"

#include "transaction_context/threadlocal_context.h"

using namespace scs;

using xdr::operator==;

TEST_CASE("int64 storage write", "[storage]")
{
	GlobalContext scs_data_structures;
	auto& script_db = scs_data_structures.contract_db;
	auto& state_db = scs_data_structures.state_db;

	auto c = test::load_wasm_from_file("cpp_contracts/test_nn_int64.wasm");
	
	auto h = hash_xdr(*c);

	REQUIRE(script_db.register_contract(h, std::move(c)));

	ThreadlocalContextStore::make_ctx(scs_data_structures);
	test::DeferredContextClear defer;

	auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

	Address a0 = hash_xdr<uint64_t>(0);

	InvariantKey k0 = hash_xdr<uint64_t>(0);

	std::unique_ptr<DeltaBatch> delta_batch = std::make_unique<DeltaBatch>();
	std::unique_ptr<TxBlock> tx_block = std::make_unique<TxBlock>();

	struct calldata_0 {
		InvariantKey key;
	};
	struct calldata_1 {
		InvariantKey key;
		uint64_t value;
	};

	auto make_set_add_tx = [&] (Address const& sender, InvariantKey const& key, int64_t set, int64_t add, bool success = true) -> Hash
	{
		struct calldata_1 {
			InvariantKey key;
			int64_t set;
			int64_t delta;
		};

		calldata_1 data {
			.key = key,
			.set = set,
			.delta = add
		};

		TransactionInvocation invocation(
			h,
			1,
			test::make_calldata(data)
		);

		Transaction tx = Transaction(sender, invocation, UINT64_MAX, 1);
		auto hash = tx_block->insert_tx(tx);

		if (success)
		{
			REQUIRE(
				exec_ctx.execute(hash, tx, *tx_block)
				== TransactionStatus::SUCCESS);
		} else
		{
			REQUIRE(
				exec_ctx.execute(hash, tx, *tx_block)
				!= TransactionStatus::SUCCESS);
		}

		exec_ctx.reset();

		return hash;
	};

	auto make_add_tx = [&] (Address const& sender, InvariantKey const& key, int64_t add, bool success = true) -> Hash
	{
		struct calldata_0 {
			InvariantKey key;
			int64_t delta;
		};

		calldata_0 data {
			.key = key,
			.delta = add
		};

		TransactionInvocation invocation(
			h,
			0,
			test::make_calldata(data)
		);
		
		Transaction tx = Transaction(sender, invocation, UINT64_MAX, 1);
		auto hash = tx_block->insert_tx(tx);

		if (success)
		{
			REQUIRE(
				exec_ctx.execute(hash, tx, *tx_block)
				== TransactionStatus::SUCCESS);
		} else
		{
			REQUIRE(
				exec_ctx.execute(hash, tx, *tx_block)
				!= TransactionStatus::SUCCESS);
		}

		exec_ctx.reset();
		
		return hash;
	};

	auto finish_block = [&] () {
		phase_merge_delta_batches(*delta_batch);
		phase_filter_deltas(scs_data_structures, *delta_batch, *tx_block);
		phase_compute_state_updates(*delta_batch, *tx_block);
		phase_finish_block(scs_data_structures, *delta_batch, *tx_block);
	};

	auto make_key = [] (Address const& addr, InvariantKey const& key) -> AddressAndKey
	{
		AddressAndKey out;
		std::memcpy(out.data(), addr.data(), sizeof(Address));

		std::memcpy(out.data() + sizeof(Address), key.data(), sizeof(InvariantKey));
		return out;
	};

	auto check_valid = [&] (const Hash& tx_hash)
	{
		REQUIRE(
			tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));
	};

	auto check_invalid = [&] (const Hash& tx_hash)
	{
		REQUIRE(
			!tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));
	};

	SECTION("write to empty slot")
	{
		SECTION("no set")
		{
			auto h1 = make_add_tx(a0, k0, -1, false);
			auto h2 = make_add_tx(a0, k0, -2, false);
			auto h3 = make_add_tx(a0, k0, 5);
			auto h4 = make_add_tx(a0, k0, 0);

			finish_block();

			check_invalid(h1);
			check_invalid(h2);
			check_valid(h3);
			check_valid(h4);

			auto hk0 = make_key(h, k0);
			auto db_val = state_db.get(hk0);

			REQUIRE(!!db_val);
			REQUIRE(db_val->nonnegative_int64() == 5);
		}

		SECTION("with set")
		{

			auto h1 = make_set_add_tx(a0, k0, 10, -1);
			auto h2 = make_set_add_tx(a0, k0, 10, -2);
			auto h3 = make_set_add_tx(a0, k0, 10, 5);
			auto h4 = make_set_add_tx(a0, k0, 10, 0);

			finish_block();

			check_valid(h1);
			check_valid(h2);
			check_valid(h3);
			check_valid(h4);

			auto hk0 = make_key(h, k0);
			auto db_val = state_db.get(hk0);

			REQUIRE(!!db_val);
			REQUIRE(db_val->nonnegative_int64() == 12);
		}

		SECTION("with negative set")
		{
			auto h1 = make_set_add_tx(a0, k0, -10, -1, false);
			auto h2 = make_set_add_tx(a0, k0, -10, -2, false);
			auto h3 = make_set_add_tx(a0, k0, -10, 5);
			auto h4 = make_set_add_tx(a0, k0, -10, 0);

			finish_block();

			check_invalid(h1);
			check_invalid(h2);
			check_valid(h3);
			check_valid(h4);

			auto hk0 = make_key(h, k0);
			auto db_val = state_db.get(hk0);

			REQUIRE(!!db_val);
			REQUIRE(db_val->nonnegative_int64() == -5);
		}
	}
}

TEST_CASE("raw mem storage write", "[storage]")
{
	GlobalContext scs_data_structures;
	auto& script_db = scs_data_structures.contract_db;
	auto& state_db = scs_data_structures.state_db;

	auto c = test::load_wasm_from_file("cpp_contracts/test_raw_memory.wasm");
	
	auto h = hash_xdr(*c);

	REQUIRE(script_db.register_contract(h, std::move(c)));

	ThreadlocalContextStore::make_ctx(scs_data_structures);
	test::DeferredContextClear defer;

	auto& exec_ctx = ThreadlocalContextStore::get_exec_ctx();

	Address a0 = hash_xdr<uint64_t>(0);

	InvariantKey k0 = hash_xdr<uint64_t>(0);

	std::unique_ptr<DeltaBatch> delta_batch = std::make_unique<DeltaBatch>();
	std::unique_ptr<TxBlock> tx_block = std::make_unique<TxBlock>();

	struct calldata_0 {
		InvariantKey key;
	};
	struct calldata_1 {
		InvariantKey key;
		uint64_t value;
	};

	auto make_transaction = [&] (Address const& sender, TransactionInvocation const& invocation) -> std::pair<Hash, Transaction>
	{
		Transaction tx = Transaction(sender, invocation, UINT64_MAX, 1);
		auto hash = tx_block->insert_tx(tx);
		return {hash, tx};
	};

	auto finish_block = [&] () {
		phase_merge_delta_batches(*delta_batch);
		phase_filter_deltas(scs_data_structures, *delta_batch, *tx_block);
		phase_compute_state_updates(*delta_batch, *tx_block);
		phase_finish_block(scs_data_structures, *delta_batch, *tx_block);
	};

	auto make_key = [] (Address const& addr, InvariantKey const& key) -> AddressAndKey
	{
		AddressAndKey out;
		std::memcpy(out.data(), addr.data(), sizeof(Address));

		std::memcpy(out.data() + sizeof(Address), key.data(), sizeof(InvariantKey));
		return out;
	};

	auto exec_success = [&] (const Hash& tx_hash, const Transaction& tx)
	{
		REQUIRE(
			exec_ctx.execute(tx_hash, tx, *tx_block)
			== TransactionStatus::SUCCESS);
	};

	auto exec_fail = [&] (const Hash& tx_hash, const Transaction& tx)
	{
		REQUIRE(
			exec_ctx.execute(tx_hash, tx, *tx_block)
			!= TransactionStatus::SUCCESS);
	};

	SECTION("write to empty slot")
	{
		calldata_1 data {
			.key = k0,
			.value = 0xAABBCCDDEEFF0011
		};

		TransactionInvocation invocation (
			h,
			1,
			test::make_calldata(data)
		);

		auto [tx_hash, tx] = make_transaction(a0, invocation);

		exec_success(tx_hash, tx);

		finish_block();

		REQUIRE(
			tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));

		auto hk0 = make_key(h, k0);

		auto db_val = state_db.get(hk0);

		REQUIRE(!!db_val);

		REQUIRE(db_val->raw_memory_storage().data == xdr::opaque_vec<RAW_MEMORY_MAX_LEN>{0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA});


		delta_batch.reset(new DeltaBatch());
		tx_block.reset(new TxBlock());

		SECTION("read key from prev block")
		{
			calldata_0 data {
				.key = k0
			};

			TransactionInvocation invocation (
				h,
				0,
				test::make_calldata(data)
			);

			auto [tx_hash, tx] = make_transaction(a0, invocation);

			exec_success(tx_hash, tx);

			auto const& logs = exec_ctx.get_logs();
			REQUIRE(logs.size() == 1);
			REQUIRE(logs[0] == std::vector<uint8_t>{0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA});
		}
		SECTION("overwrite key")
		{
			calldata_1 data {
				.key = k0,
				.value = 0xA0B0C0D0E0F00010
			};

			TransactionInvocation invocation (
				h,
				1,
				test::make_calldata(data)
			);

			auto [tx_hash, tx] = make_transaction(a0, invocation);

			exec_success(tx_hash, tx);

			finish_block();

			REQUIRE(
				tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));

			auto hk0 = make_key(h, k0);

			auto db_val = state_db.get(hk0);

			REQUIRE(!!db_val);

			REQUIRE(db_val->raw_memory_storage().data == xdr::opaque_vec<RAW_MEMORY_MAX_LEN>{0x10, 0x00, 0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0});
		}

		SECTION("delete_first key solo")
		{
			calldata_0 data {
				.key = k0
			};

			TransactionInvocation invocation (
				h,
				4,
				test::make_calldata(data)
			);

			auto [tx_hash, tx] = make_transaction(a0, invocation);

			exec_success(tx_hash, tx);

			finish_block();

			REQUIRE(
				tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));

			auto hk0 = make_key(h, k0);

			auto db_val = state_db.get(hk0);

			REQUIRE(!db_val);
		}

		SECTION("delete_last key solo")
		{
			calldata_0 data {
				.key = k0
			};

			TransactionInvocation invocation (
				h,
				5,
				test::make_calldata(data)
			);

			auto [tx_hash, tx] = make_transaction(a0, invocation);

			exec_success(tx_hash, tx);

			finish_block();

			REQUIRE(
				tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));

			auto hk0 = make_key(h, k0);

			auto db_val = state_db.get(hk0);

			REQUIRE(!db_val);
		}

		SECTION("delete_last + write ok")
		{
			calldata_0 data {
				.key = k0
			};

			TransactionInvocation invocation (
				h,
				5,
				test::make_calldata(data)
			);

			auto [tx_hash, tx] = make_transaction(a0, invocation);

			exec_success(tx_hash, tx);

			exec_ctx.reset();

			calldata_1 data2 {
				.key = k0,
				.value = 0xA0B0C0D0E0F00010
			};

			TransactionInvocation invocation2 (
				h,
				1,
				test::make_calldata(data2)
			);

			auto [tx_hash2, tx2] = make_transaction(a0, invocation2);

			exec_success(tx_hash2, tx2);

			finish_block();

			REQUIRE(
				tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));
			REQUIRE(
				tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash2));

			auto hk0 = make_key(h, k0);

			auto db_val = state_db.get(hk0);

			REQUIRE(!db_val);
		}

		SECTION("delete_first + write bad")
		{
			calldata_0 data {
				.key = k0
			};

			TransactionInvocation invocation (
				h,
				4,
				test::make_calldata(data)
			);

			auto [tx_hash, tx] = make_transaction(a0, invocation);

			exec_success(tx_hash, tx);

			calldata_1 data2 {
				.key = k0,
				.value = 0xA0B0C0D0E0F00010
			};

			TransactionInvocation invocation2 (
				h,
				1,
				test::make_calldata(data2)
			);

			auto [tx_hash2, tx2] = make_transaction(a0, invocation2);

			exec_ctx.reset();

			exec_success(tx_hash2, tx2);

			finish_block();

			REQUIRE(
				tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));
			REQUIRE(
				!tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash2));

			auto hk0 = make_key(h, k0);

			auto db_val = state_db.get(hk0);

			REQUIRE(!db_val);
		}
	}

	SECTION("get from empty slot")
	{
		calldata_0 data {
			.key = k0
		};

		TransactionInvocation invocation (
			h,
			0,
			test::make_calldata(data)
		);

		auto [tx_hash, tx] = make_transaction(a0, invocation);

		exec_fail(tx_hash, tx);
	}

	SECTION("double write fail")
	{
		calldata_0 data {
			.key = k0
		};

		TransactionInvocation invocation (
			h,
			2,
			test::make_calldata(data)
		);

		auto [tx_hash, tx] = make_transaction(a0, invocation);

		// fails because it tries to call set() on one mem location twice in a tx
		exec_fail(tx_hash, tx);

		{
			auto const& logs = exec_ctx.get_logs();
			REQUIRE(logs.size() == 1);
			REQUIRE(logs[0] == std::vector<uint8_t>{0x34, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
		}
		tx_block -> template invalidate<TransactionFailurePoint::COMPUTE>(tx_hash);

		finish_block();

		REQUIRE(
			!tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));

		auto hk0 = make_key(h, k0);

		auto db_val = state_db.get(hk0);

		REQUIRE(!db_val);
	}

	SECTION("read own writes")
	{
		calldata_0 data {
			.key = k0
		};

		TransactionInvocation invocation (
			h,
			3,
			test::make_calldata(data)
		);

		auto [tx_hash, tx] = make_transaction(a0, invocation);

		exec_success(tx_hash, tx);

		{
			auto const& logs = exec_ctx.get_logs();
			REQUIRE(logs.size() == 1);
			REQUIRE(logs[0] == std::vector<uint8_t>{0x34, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
		}

		finish_block();

		REQUIRE(
			tx_block->is_valid(TransactionFailurePoint::FINAL, tx_hash));

		auto hk0 = make_key(h, k0);

		auto db_val = state_db.get(hk0);

		REQUIRE(!!db_val);

		REQUIRE(db_val->raw_memory_storage().data == xdr::opaque_vec<RAW_MEMORY_MAX_LEN>{0x34, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
	}
}
