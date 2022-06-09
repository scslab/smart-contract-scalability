#include <catch2/catch_test_macros.hpp>

#include "tx_block/tx_block.h"

#include "object/object_mutator.h"
#include "state_db/delta_vec.h"

#include "xdr/storage_delta.h"
#include "xdr/transaction.h"

using xdr::operator==;

namespace scs
{

using raw_mem_val = xdr::opaque_vec<RAW_MEMORY_MAX_LEN>;

TEST_CASE("raw mem only", "[mutator]")
{
	TxBlock txs;

	// just need different hashes
	Transaction tx1, tx2;
	tx1.gas_limit = 1;
	tx2.gas_limit = 2;

	auto h1 = txs.insert_tx(tx1);
	auto h2 = txs.insert_tx(tx2);

	REQUIRE(h1 != h2);

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
		out.type(DeltaType::DELETE_LAST);
		return out;
	};
	auto make_raw_mem_write = [] (raw_mem_val b)
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

	auto check_invalid = [&] (Hash const& h, TransactionFailurePoint p = TransactionFailurePoint::FINAL)
	{
		REQUIRE(!txs.is_valid(p, h));
	};


	DeltaVector vec;
	ObjectMutator base;

	raw_mem_val val1 = {0x01, 0x02, 0x03, 0x04};
	raw_mem_val val2 = {0x01, 0x03, 0x05, 0x07};

	SECTION("no base obj")
	{
		SECTION("one delete")
		{
			vec.add_delta(make_delete_delta(), make_priority(h1, 1));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);

			base.apply_valid_deltas(vec, txs);

			REQUIRE(base.get_object() == std::nullopt);
		}

		SECTION("one write")
		{
			vec.add_delta(make_raw_mem_write(val1), make_priority(h1, 1));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);

			base.apply_valid_deltas(vec, txs);

			StorageObject expect;
			expect.type(RAW_MEMORY);
			expect.raw_memory_storage().data = val1;
			REQUIRE((bool)base.get_object());
			if (base.get_object())
			{
				REQUIRE(*base.get_object() == expect);
			}
		}
		SECTION("two write different")
		{
			vec.add_delta(make_raw_mem_write(val1), make_priority(h1, 1));
			vec.add_delta(make_raw_mem_write(val2), make_priority(h2, 2));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_invalid(h1);
			check_valid(h2);

			base.apply_valid_deltas(vec, txs);

			StorageObject expect;
			expect.type(RAW_MEMORY);
			expect.raw_memory_storage().data = val2;
			REQUIRE((bool)base.get_object());
			if (base.get_object())
			{
				REQUIRE(*base.get_object() == expect);
			}
		}

		SECTION("two write different other order")
		{
			vec.add_delta(make_raw_mem_write(val1), make_priority(h1, 3));
			vec.add_delta(make_raw_mem_write(val2), make_priority(h2, 2));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);
			check_invalid(h2);

			base.apply_valid_deltas(vec, txs);

			StorageObject expect;
			expect.type(RAW_MEMORY);
			expect.raw_memory_storage().data = val1;
			REQUIRE((bool)base.get_object());
			if (base.get_object())
			{
				REQUIRE(*base.get_object() == expect);
			}
		}
		SECTION("two write same")
		{
			vec.add_delta(make_raw_mem_write(val1), make_priority(h1, 3));
			vec.add_delta(make_raw_mem_write(val1), make_priority(h2, 2));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);
			check_valid(h2);

			base.apply_valid_deltas(vec, txs);

			StorageObject expect;
			expect.type(RAW_MEMORY);
			expect.raw_memory_storage().data = val1;
			REQUIRE((bool)base.get_object());
			if (base.get_object())
			{
				REQUIRE(*base.get_object() == expect);
			}
		}
		SECTION("write and delete")
		{
			vec.add_delta(make_raw_mem_write(val1), make_priority(h1, 3));
			vec.add_delta(make_delete_delta(), make_priority(h2, 2));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);
			check_valid(h2);

			base.apply_valid_deltas(vec, txs);

			REQUIRE(!base.get_object());
		}
		SECTION("write and delete swap prio")
		{
			std::printf("start test\n");
			vec.add_delta(make_raw_mem_write(val1), make_priority(h1, 1));
			vec.add_delta(make_delete_delta(), make_priority(h2, 2));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			std::printf("done filter\n");
			check_valid(h1);
			check_valid(h2);

			base.apply_valid_deltas(vec, txs);

			REQUIRE(!base.get_object());
		}
	}
	SECTION("populate with something")
	{
		StorageObject obj;
		obj.type(RAW_MEMORY);
		obj.raw_memory_storage().data = val1;

		base.populate_base(obj);

		SECTION("write one same")
		{
			vec.add_delta(make_raw_mem_write(val1), make_priority(h1, 3));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);

			base.apply_valid_deltas(vec, txs);

			REQUIRE((bool)base.get_object());
			REQUIRE(*base.get_object() == obj);
		}

		SECTION("write one diff")
		{
			vec.add_delta(make_raw_mem_write(val2), make_priority(h1, 3));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);

			base.apply_valid_deltas(vec, txs);

			obj.raw_memory_storage().data = val2;

			REQUIRE((bool)base.get_object());
			REQUIRE(*base.get_object() == obj);
		}

		SECTION("one delete")
		{
			vec.add_delta(make_delete_delta(), make_priority(h1, 3));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);

			base.apply_valid_deltas(vec, txs);

			REQUIRE(!base.get_object());
		}

		SECTION("two writes of same (different from init) value")
		{
			vec.add_delta(make_raw_mem_write(val2), make_priority(h1, 3));
			vec.add_delta(make_raw_mem_write(val2), make_priority(h2, 3));


			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);
			check_valid(h2);

			base.apply_valid_deltas(vec, txs);

			obj.raw_memory_storage().data = val2;

			REQUIRE((bool)base.get_object());
			REQUIRE(*base.get_object() == obj);
		}
		SECTION("two writes of diff values")
		{
			vec.add_delta(make_raw_mem_write(val2), make_priority(h1, 3));
			vec.add_delta(make_raw_mem_write(val1), make_priority(h2, 2));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);
			check_invalid(h2);

			base.apply_valid_deltas(vec, txs);

			obj.raw_memory_storage().data = val2;

			REQUIRE((bool)base.get_object());
			REQUIRE(*base.get_object() == obj);
		}
	}
}

} /* scs */
