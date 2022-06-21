#include <catch2/catch_test_macros.hpp>

#include "object/make_delta.h"

#include "storage_proxy/proxy_applicator.h"

#include "xdr/storage_delta.h"
#include "xdr/transaction.h"

using xdr::operator==;

namespace scs
{

using raw_mem_val = xdr::opaque_vec<RAW_MEMORY_MAX_LEN>;

TEST_CASE("raw mem only", "[mutator]")
{
	std::unique_ptr<ProxyApplicator> applicator;

	auto check_valid = [&] (StorageDelta const& d)
	{
		REQUIRE(applicator -> try_apply(d));
	};

	auto check_invalid = [&] (StorageDelta const& d)
	{
		REQUIRE(!applicator -> try_apply(d));
	};

	auto make_raw_mem_write = [&] (const raw_mem_val& v) -> StorageDelta
	{
		raw_mem_val copy = v;
		return make_raw_memory_write(std::move(copy));
	};

	auto expect_valence = [&] (TypeclassValence tcv)
	{
		REQUIRE(applicator->get_tc().get_valence().tv.type() == tcv);
	};

	auto tc_expect_mem_value = [&] (raw_mem_val v)
	{
		REQUIRE(applicator->get_tc().get_valence().tv.data() == v);
	};

	auto tc_expect_deleted_last = [&] ()
	{
		REQUIRE(applicator->get_tc().get_valence().deleted_last == 1);
	};

	auto val_expect_mem_value = [&] (raw_mem_val v)
	{
		REQUIRE(applicator->get());
		REQUIRE(applicator->get()->raw_memory_storage().data == v);
	};

	auto val_expect_nullopt = [&] ()
	{
		REQUIRE(!applicator->get());
	};

	raw_mem_val val1 = {0x01, 0x02, 0x03, 0x04};
	raw_mem_val val2 = {0x01, 0x03, 0x05, 0x07};

	SECTION("no base obj")
	{
		applicator = std::make_unique<ProxyApplicator>(std::nullopt);

		SECTION("one delete first")
		{
			check_valid(make_delete_first());

			REQUIRE(applicator->get() == std::nullopt);
			expect_valence(TypeclassValence::TV_DELETE_FIRST);
		}

		SECTION("one delete last")
		{
			check_valid(make_delete_last());

			REQUIRE(applicator->get() == std::nullopt);
			expect_valence(TypeclassValence::TV_FREE);
		}

		SECTION("delete first and last")
		{
			check_valid(make_delete_first());
			check_valid(make_delete_last());

			REQUIRE(applicator->get() == std::nullopt);
			expect_valence(TypeclassValence::TV_DELETE_FIRST);
			tc_expect_deleted_last();
		}

		SECTION("delete last and first")
		{
			check_valid(make_delete_last());
			check_valid(make_delete_first());

			REQUIRE(applicator->get() == std::nullopt);
			expect_valence(TypeclassValence::TV_DELETE_FIRST);
			tc_expect_deleted_last();
		}

		SECTION("one write")
		{
			check_valid(make_raw_mem_write(val1));

			expect_valence(TypeclassValence::TV_RAW_MEMORY_WRITE);
			tc_expect_mem_value(val1);
			val_expect_mem_value(val1);
		}
		SECTION("two write different")
		{
			check_valid(make_raw_mem_write(val1));

			expect_valence(TypeclassValence::TV_RAW_MEMORY_WRITE);
			tc_expect_mem_value(val1);
			val_expect_mem_value(val1);

			check_invalid(make_raw_mem_write(val2));

			expect_valence(TypeclassValence::TV_RAW_MEMORY_WRITE);
			tc_expect_mem_value(val1);
			val_expect_mem_value(val1);
		}

		SECTION("two write same")
		{
			check_valid(make_raw_mem_write(val1));

			expect_valence(TypeclassValence::TV_RAW_MEMORY_WRITE);
			tc_expect_mem_value(val1);
			val_expect_mem_value(val1);


			check_valid(make_raw_mem_write(val1));

			expect_valence(TypeclassValence::TV_RAW_MEMORY_WRITE);
			tc_expect_mem_value(val1);
			val_expect_mem_value(val1);
		}

		SECTION("write and delete")
		{

			check_valid(make_raw_mem_write(val1));
			check_valid(make_delete_last());

			expect_valence(TypeclassValence::TV_RAW_MEMORY_WRITE);
			tc_expect_deleted_last();
			tc_expect_mem_value(val1);
			val_expect_nullopt();

		}
		SECTION("write and delete swap order")
		{
			// different from regular typeclass rules,
			// proxyapplicator blocks everything after a delete.
			check_valid(make_delete_last());
			check_invalid(make_raw_mem_write(val1));

			expect_valence(TypeclassValence::TV_FREE);
			tc_expect_deleted_last();
			val_expect_nullopt();
		}
	}
	SECTION("populate with something")
	{
		StorageObject obj;
		obj.type(RAW_MEMORY);
		obj.raw_memory_storage().data = val1;

		applicator = std::make_unique<ProxyApplicator>(obj);

		SECTION("write one same")
		{
			check_valid(make_raw_mem_write(val1));

			expect_valence(TypeclassValence::TV_RAW_MEMORY_WRITE);

			tc_expect_mem_value(val1);
			val_expect_mem_value(val1);
		}

		SECTION("write one diff")
		{
			check_valid(make_raw_mem_write(val2));

			expect_valence(TypeclassValence::TV_RAW_MEMORY_WRITE);

			tc_expect_mem_value(val2);
			val_expect_mem_value(val2);
		}

		SECTION("one delete last")
		{
			check_valid(make_delete_last());

			expect_valence(TypeclassValence::TV_FREE);
			tc_expect_deleted_last();
			val_expect_nullopt();
		}
		SECTION("one delete first")
		{

			check_valid(make_delete_first());

			expect_valence(TypeclassValence::TV_DELETE_FIRST);
			val_expect_nullopt();
		}

		SECTION("two writes of same (different from init) value")
		{
			check_valid(make_raw_mem_write(val2));
			check_valid(make_raw_mem_write(val2));

			expect_valence(TypeclassValence::TV_RAW_MEMORY_WRITE);

			tc_expect_mem_value(val2);
			val_expect_mem_value(val2);
		}
		SECTION("two writes of diff values")
		{
			check_valid(make_raw_mem_write(val2));
			check_invalid(make_raw_mem_write(val1));

			expect_valence(TypeclassValence::TV_RAW_MEMORY_WRITE);

			tc_expect_mem_value(val2);
			val_expect_mem_value(val2);
		} 
	} 
}
/*
TEST_CASE("nonnegative int64 only", "[mutator]")
{
	TxBlock txs;

	// just need different hashes
	Transaction tx1, tx2, tx3;
	tx1.gas_limit = 1;
	tx2.gas_limit = 2;
	tx3.gas_limit = 3;

	auto h1 = txs.insert_tx(tx1);
	auto h2 = txs.insert_tx(tx2);
	auto h3 = txs.insert_tx(tx3);

	REQUIRE(h1 != h2);
	REQUIRE(h1 != h3);
	REQUIRE(h2 != h3);

	auto make_priority = [] (Hash const& h, uint64_t p) -> DeltaPriority
	{
		DeltaPriority out;
		out.tx_hash = h;
		out.custom_priority = p;
		return out;
	};

	auto make_set_add_delta = [] (int64_t set_value, int64_t delta_value)
	{
		return make_nonnegative_int64_set_add(set_value, delta_value);
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

	auto make_expect = [] (int64_t val)
	{
		StorageObject obj;
		obj.type(ObjectType::NONNEGATIVE_INT64);
		obj.nonnegative_int64() = val;
		return obj;
	};

	SECTION("no base obj")
	{
		SECTION("strict negative")
		{
			vec.add_delta(make_set_add_delta(0, -1), make_priority(h1, 1));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_invalid(h1);

			base.apply_valid_deltas(vec, txs);

			REQUIRE(!base.get_object());
		}
		SECTION("pos ok")
		{
			vec.add_delta(make_set_add_delta(0, 1), make_priority(h1, 1));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);

			base.apply_valid_deltas(vec, txs);

			REQUIRE(!!base.get_object());
			REQUIRE(*base.get_object() == make_expect(1));
		}
		SECTION("pos sub with set ok")
		{
			vec.add_delta(make_set_add_delta(10, -5), make_priority(h1, 1));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);

			base.apply_valid_deltas(vec, txs);

			REQUIRE(!!base.get_object());
			REQUIRE(*base.get_object() == make_expect(5));
		}
		SECTION("neg add with set ok")
		{
			vec.add_delta(make_set_add_delta(-10, 5), make_priority(h1, 1));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);

			base.apply_valid_deltas(vec, txs);

			REQUIRE(!!base.get_object());
			REQUIRE(*base.get_object() == make_expect(-5));
		}
		SECTION("neg add with neg bad")
		{
			vec.add_delta(make_set_add_delta(-10, -5), make_priority(h1, 1));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_invalid(h1);

			base.apply_valid_deltas(vec, txs);

			REQUIRE(!base.get_object());
		}

		SECTION("conflicting sets bad")
		{
			vec.add_delta(make_set_add_delta(10, -5), make_priority(h1, 1));
			vec.add_delta(make_set_add_delta(15, -5), make_priority(h2, 2));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_invalid(h1);
			check_valid(h2);

			base.apply_valid_deltas(vec, txs);

			REQUIRE(!!base.get_object());
			REQUIRE(*base.get_object() == make_expect(10));
		}

		SECTION("conflicting subs bad")
		{
			vec.add_delta(make_set_add_delta(10, -5), make_priority(h1, 1));
			vec.add_delta(make_set_add_delta(10, -5), make_priority(h2, 2));
			vec.add_delta(make_set_add_delta(10, -1), make_priority(h3, 0));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);
			check_valid(h2);
			check_invalid(h3);

			base.apply_valid_deltas(vec, txs);

			REQUIRE(!!base.get_object());
			REQUIRE(*base.get_object() == make_expect(0));
		}

		SECTION("invalid single delta")
		{
			vec.add_delta(make_set_add_delta(10, -11), make_priority(h1, 1));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_invalid(h1);

			base.apply_valid_deltas(vec, txs);

			REQUIRE(!base.get_object());
		}

		SECTION("check overflow multi subs")
		{
			vec.add_delta(make_set_add_delta(INT64_MAX, INT64_MIN + 10), make_priority(h1, 1));
			vec.add_delta(make_set_add_delta(INT64_MAX, -10), make_priority(h2, 0));

			base.template filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(vec, txs);

			check_valid(h1);
			check_invalid(h2);

			base.apply_valid_deltas(vec, txs);

			REQUIRE(!!base.get_object());
			REQUIRE(*base.get_object() == make_expect(9));
		}
	}
} */

} /* scs */
