#include <catch2/catch_test_macros.hpp>

#include "object/make_delta.h"

#include "object/delta_type_class.h"

#include "xdr/storage_delta.h"

using xdr::operator==;

namespace scs
{

using raw_mem_val = xdr::opaque_vec<RAW_MEMORY_MAX_LEN>;

TEST_CASE("add storage deltas", "[typeclass]")
{
	DeltaTypeClass dtc;

	raw_mem_val val1 = {0x01, 0x02, 0x03, 0x04};
	raw_mem_val val2 = {0x01, 0x03, 0x05, 0x07};

	auto make_raw_mem_write = [&] (const raw_mem_val& v) -> StorageDelta
	{
		raw_mem_val copy = v;
		return make_raw_memory_write(std::move(copy));
	};

	auto require_accept = [&] (const StorageDelta& d)
	{
		REQUIRE(dtc.can_accept(d));
	};

	auto require_reject = [&] (const StorageDelta& d)
	{
		REQUIRE(!dtc.can_accept(d));
	};

	auto expect_valence_free = [&]
	{
		REQUIRE(dtc.get_valence().tv.type() == TypeclassValence::TV_FREE);
	};

	auto expect_valence_mem = [&] (raw_mem_val const& val)
	{
		REQUIRE(dtc.get_valence().tv.type() == TypeclassValence::TV_RAW_MEMORY_WRITE);
		REQUIRE(dtc.get_valence().tv.data() == val);
	};

	auto expect_valence_error = [&]
	{
		REQUIRE(dtc.get_valence().tv.type() == TypeclassValence::TV_ERROR);
	};

	auto expect_delete_first = [&]
	{
		REQUIRE(dtc.get_valence().tv.type() == TypeclassValence::TV_DELETE_FIRST);
	};

	auto expect_delete_last = [&]
	{
		REQUIRE(dtc.get_valence().deleted_last == 1);
	};

	SECTION("delete interactions")
	{
		SECTION("first")
		{
			require_accept(make_delete_first());
			expect_valence_free();

			dtc.add(make_delete_first());
			expect_delete_first();
		}
		SECTION("last")
		{
			require_accept(make_delete_last());
			expect_valence_free();

			dtc.add(make_delete_last());
			expect_delete_last();
		}
		SECTION("first and last")
		{
			dtc.add(make_delete_last());
			dtc.add(make_delete_first());
			expect_delete_last();
			expect_delete_first();
		}
	}
	SECTION("raw mem writes")
	{
		SECTION("one write")
		{
			require_accept(make_raw_mem_write(val1));

			dtc.add(make_raw_mem_write(val1));
			expect_valence_mem(val1);
			SECTION("same write")
			{
				dtc.add(make_raw_mem_write(val1));
				expect_valence_mem(val1);
			}
		}
		SECTION("conflicting writes")
		{
			require_accept(make_raw_mem_write(val1));

			dtc.add(make_raw_mem_write(val1));
			expect_valence_mem(val1);

			require_accept(make_raw_mem_write(val1));
			require_reject(make_raw_mem_write(val2));

			dtc.add(make_raw_mem_write(val2));

			expect_valence_error();
		}
	}

}

} /* scs */
