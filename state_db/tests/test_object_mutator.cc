#include <catch2/catch_test_macros.hpp>

#include "tx_block/tx_block.h"

#include "state_db/object_mutator.h"
#include "state_db/delta_vec.h"

#include "xdr/storage_delta.h"
#include "xdr/transaction.h"

namespace scs
{

TEST_CASE("raw mem only", "[mutator]")
{
	TxBlock txs;

	// just need different hashes
	Transaction tx1, tx2, tx3;
	tx1.gas_limit = 1;
	tx2.gas_limit = 2;
	tx3.gas_limit = 3;

	auto h1 = txs.insert_tx(tx1);

	auto make_priority = [] (Hash const& h, uint64_t p) -> DeltaPriority
	{
		DeltaPriority out;
		out.tx_hash = h;
		out.custom_priority = p;
		return out;
	};

	auto make_delete_delta = [] 
	{
		StorageDelta out;
		out.type(DeltaType::DELETE);
		return out;
	};
	auto make_raw_mem_write = [] (xdr::opaque_vec<RAW_MEMORY_MAX_LEN> b)
	{
		StorageDelta out;
		out.type(DeltaType::RAW_MEMORY_WRITE);
		out.data() = b;
		return out;
	};

	auto check_valid = [&] (Hash const& h, TransactionFailurePoint p = TransactionFailurePoint::FINAL)
	{
		REQUIRE(txs.is_valid(p, h));
	};

	DeltaVector vec;
	ObjectMutator base;

	SECTION("one delete")
	{
		vec.add_delta(make_delete_delta(), make_priority(h1, 1));

		base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

		check_valid(h1);

		REQUIRE(base.get_object() == std::nullopt);
	}





}

} /* scs */
