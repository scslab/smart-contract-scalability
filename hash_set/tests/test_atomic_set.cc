#include <catch2/catch_test_macros.hpp>

#include "crypto/hash.h"
#include "hash_set/atomic_set.h"

namespace scs
{

TEST_CASE("insert atomic set", "[hashset]")
{
	AtomicSet set(10);

	auto good_insert = [&] (uint64_t i)
	{
		REQUIRE(set.try_insert(hash_xdr(i)));
	};

	auto bad_insert = [&] (uint64_t i)
	{
		REQUIRE(!set.try_insert(hash_xdr(i)));
	};

	SECTION("distinct")
	{
		good_insert(0);
		good_insert(1);
		good_insert(2);
	}

	SECTION("same")
	{
		good_insert(0);
		good_insert(1);
		bad_insert(1);
		bad_insert(0);
		good_insert(2);
	}
}

TEST_CASE("insert too small", "[hashset]")
{
	AtomicSet set(1);

	auto good_insert = [&] (uint64_t i)
	{
		REQUIRE(set.try_insert(hash_xdr(i)));
	};

	auto insert_several = [&] (uint64_t start, uint32_t count)
	{
		for (auto i = 0u; i < count; i++)
		{
			set.try_insert(hash_xdr(start + i));
		}
	};

	SECTION("no resize")
	{
		good_insert(0);
		REQUIRE_THROWS(set.try_insert(hash_xdr(1)));
	}

	SECTION("resize bigger")
	{
		set.resize(5);
		good_insert(0);
		good_insert(1);
		good_insert(2);
		good_insert(3);
		good_insert(4);

		REQUIRE_THROWS(insert_several(5, 5));
	}
}

}