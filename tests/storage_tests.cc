#include <catch2/catch_test_macros.hpp>

#include "transaction_context/global_context.h"

#include "phase/phases.h"

#include "crypto/hash.h"
#include "test_utils/load_wasm.h"
#include "test_utils/make_calldata.h"

#include "transaction_context/threadlocal_context.h"

using namespace scs;

using xdr::operator==;

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

	DeltaBatch delta_batch;
	TxBlock tx_block;

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
		auto hash = tx_block.insert_tx(tx);
		return {hash, tx};
	};

	auto finish_block = [&] () {
		phase_merge_delta_batches(delta_batch);
		phase_filter_deltas(scs_data_structures, delta_batch, tx_block);
		phase_finish_block(scs_data_structures, delta_batch, tx_block);
	};

	auto make_key = [] (Address const& addr, InvariantKey const& key) -> AddressAndKey
	{
		AddressAndKey out;
		std::memcpy(out.data(), addr.data(), sizeof(Address));

		std::memcpy(out.data() + sizeof(Address), key.data(), sizeof(InvariantKey));
		return out;
	};

	SECTION("write to empty slot")
	{
		calldata_1 data {
			.key = k0,
			.value = 0xAABBCCDDEEFF0011
		};

		TransactionInvocation invocation (
			h,
			0,
			test::make_calldata(data)
		);

		auto [tx_hash, tx] = make_transaction(a0, invocation);

		REQUIRE(
			exec_ctx.execute(tx)
			== TransactionStatus::SUCCESS);

		finish_block();

		REQUIRE(
			tx_block.is_valid(TransactionFailurePoint::FINAL, tx_hash));

		auto a0k0 = make_key(a0, k0);

		auto db_val = state_db.get(a0k0);

		REQUIRE(!!db_val);

		REQUIRE(db_val->raw_memory_storage().data == xdr::opaque_vec<RAW_MEMORY_MAX_LEN>{0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA});
	}
}
