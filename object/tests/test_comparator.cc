#include <catch2/catch_test_macros.hpp>

#include "object/comparators.h"

#include "xdr/storage_delta.h"

#include "state_db/delta_vec.h"

#include "crypto/hash.h"

using namespace scs;

using xdr::operator==;

// just a few tests so I don't repeat the same
// comparator <=> weird bugs that
// caused stuff to get dropped from delta_vec
// something like x < y and x > y implies x == y within an std::set or something like that

TEST_CASE("priority", "[comparator]")
{
	auto make_priority = [] (Hash const& h, uint64_t p) -> DeltaPriority
	{
		DeltaPriority out;
		out.tx_hash = h;
		out.custom_priority = p;
		return out;
	};

	auto h0 = hash_xdr<uint64_t>(0);
	auto h1 = hash_xdr<uint64_t>(1);

	SECTION("diff priority")
	{
		auto p1 = make_priority(h0, 1);
		auto p2 = make_priority(h1, 2);

		REQUIRE(p1 < p2);
		REQUIRE(!(p1 > p2));
	}
	SECTION("same priority")
	{
		auto p1 = make_priority(h0, 1);
		auto p2 = make_priority(h1, 1);

		// h0 < h1
		REQUIRE(p1 != p2);
		REQUIRE((h0 < h1) == (p1 < p2));
	}
}

TEST_CASE("type priority pair", "[comparator]")
{
	auto h = hash_xdr<uint64_t>(0);

	auto make_priority = [] (Hash const& h, uint64_t p, DeltaType t)
	{
		DeltaPriority out;
		out.tx_hash = h;
		out.custom_priority = p;
		StorageDelta d;
		d.type(t);
		return std::make_pair<StorageDelta, DeltaPriority>(std::move(d), std::move(out));
	};


	SECTION("same types, non delete")
	{
		auto p1 = make_priority(h, 0, DeltaType::RAW_MEMORY_WRITE);
		auto p2 = make_priority(h, 1, DeltaType::RAW_MEMORY_WRITE);

		REQUIRE( p1 < p2);
		REQUIRE(!(p1 > p2));
	}

	SECTION("delete first goes first")
	{
		auto p1 = make_priority(h, 0, DeltaType::DELETE_FIRST);
		auto p2 = make_priority(h, 1, DeltaType::RAW_MEMORY_WRITE);
		auto p3 = make_priority(h, 4, DeltaType::DELETE_FIRST);

		REQUIRE(p1 > p2);
		REQUIRE(p3 > p1);
		REQUIRE(p3 > p2);
	}

	SECTION("delete last goes last")
	{
		auto p1 = make_priority(h, 0, DeltaType::DELETE_LAST);
		auto p2 = make_priority(h, 10, DeltaType::RAW_MEMORY_WRITE);
		auto p3 = make_priority(h, 4, DeltaType::DELETE_LAST);
		auto p4 = make_priority(h, 2, DeltaType::DELETE_FIRST);

		REQUIRE(p3 > p1);
		REQUIRE(p2 > p3);
		REQUIRE(p2 > p1);

		REQUIRE(p4 > p2);
		REQUIRE(p4 > p1);
		REQUIRE(p4 > p3);
	}
}

TEST_CASE("test vector", "[comparator]")
{
	auto h = hash_xdr<uint64_t>(0);

	auto make_priority = [] (Hash const& h, uint64_t p, DeltaType t)
	{
		DeltaPriority out;
		out.tx_hash = h;
		out.custom_priority = p;
		StorageDelta d;
		d.type(t);
		return std::make_pair<StorageDelta, DeltaPriority>(std::move(d), std::move(out));
	};

	SECTION("insert all different keys")
	{
		auto [d1, p1] = make_priority(h, 1, DeltaType::DELETE_FIRST);
		auto [d2, p2] = make_priority(h, 5, DeltaType::RAW_MEMORY_WRITE);
		auto [d3, p3] = make_priority(h, 10, DeltaType::DELETE_LAST);

		DeltaVector vec;

		SECTION("insert ordering forwards")
		{
			vec.add_delta(std::move(d1), std::move(p1));

			REQUIRE(vec.size() == 1);
			REQUIRE(vec.get_sorted_deltas().begin()->first.type() == DeltaType::DELETE_FIRST);

			vec.add_delta(std::move(d2), std::move(p2));

			REQUIRE(vec.size() == 2);
			REQUIRE(vec.get_sorted_deltas().begin()->first.type() == DeltaType::DELETE_FIRST);

			vec.add_delta(std::move(d3), std::move(p3));

			REQUIRE(vec.size() == 3);
			REQUIRE(vec.get_sorted_deltas().begin()->first.type() == DeltaType::DELETE_FIRST);
		}

		SECTION("insert ordering backwards")
		{
			vec.add_delta(std::move(d3), std::move(p3));

			REQUIRE(vec.size() == 1);
			REQUIRE(vec.get_sorted_deltas().begin()->first.type() == DeltaType::DELETE_LAST);

			vec.add_delta(std::move(d2), std::move(p2));

			REQUIRE(vec.size() == 2);
			REQUIRE(vec.get_sorted_deltas().begin()->first.type() == DeltaType::RAW_MEMORY_WRITE);

			vec.add_delta(std::move(d1), std::move(p1));

			REQUIRE(vec.size() == 3);
			REQUIRE(vec.get_sorted_deltas().begin()->first.type() == DeltaType::DELETE_FIRST);
		}
	}
}
