#include <catch2/catch_test_macros.hpp>

#include "hash_set/utils.h"

#include "crypto/hash.h"

namespace scs
{

TEST_CASE("clear", "[hs_utils]")
{
	HashSet hs;

	auto add = [&] (uint64_t threshold)
	{
		hs.hashes.push_back(HashSetEntry(hash_xdr<uint64_t>(threshold), threshold));
	};


	SECTION("no elts")
	{
		normalize_hashset(hs);
		SECTION("low threshold")
		{
			clear_hashset(hs, 0);

			REQUIRE(hs.hashes.size() == 0);
		}
		SECTION("high threshold")
		{
			clear_hashset(hs, 10);
			REQUIRE(hs.hashes.size() == 0);
		}
	}
	SECTION("all different thresholds, even")
	{
		add(0);
		add(1);
		add(10);
		add(11);

		normalize_hashset(hs);

		SECTION("low")
		{
			clear_hashset(hs, 0);
			REQUIRE(hs.hashes.size() == 3);
		}
		SECTION("med")
		{
			clear_hashset(hs, 5);
			REQUIRE(hs.hashes.size() == 2);
		}
		SECTION("higher")
		{
			clear_hashset(hs, 10);
			REQUIRE(hs.hashes.size() == 1);
		}
		SECTION("top")
		{
			clear_hashset(hs, 11);
			REQUIRE(hs.hashes.size() == 0);
		}
		SECTION("above top")
		{
			clear_hashset(hs, UINT64_MAX);
			REQUIRE(hs.hashes.size() == 0);
		}
	}
	SECTION("all different thresholds, odd")
	{
		add(0);
		add(1);
		add(5);
		add(10);
		add(11);

		normalize_hashset(hs);

		SECTION("low")
		{
			clear_hashset(hs, 0);
			REQUIRE(hs.hashes.size() == 4);
		}
		SECTION("med")
		{
			clear_hashset(hs, 5);
			REQUIRE(hs.hashes.size() == 2);
		}
		SECTION("higher")
		{
			clear_hashset(hs, 10);
			REQUIRE(hs.hashes.size() == 1);
		}
		SECTION("top")
		{
			clear_hashset(hs, 11);
			REQUIRE(hs.hashes.size() == 0);
		}
		SECTION("above top")
		{
			clear_hashset(hs, UINT64_MAX);
			REQUIRE(hs.hashes.size() == 0);
		}
	}
}




}