#include <catch2/catch_test_macros.hpp>

#include "object/make_delta.h"

#include "storage_proxy/proxy_applicator.h"

#include "crypto/hash.h"

#include "xdr/storage_delta.h"
#include "xdr/transaction.h"

using xdr::operator==;

namespace scs {

using raw_mem_val = xdr::opaque_vec<RAW_MEMORY_MAX_LEN>;

TEST_CASE("raw mem only", "[mutator]")
{
    std::unique_ptr<ProxyApplicator> applicator;

    auto check_valid
        = [&](StorageDelta const& d) { REQUIRE(applicator->try_apply(d)); };

    auto check_invalid
        = [&](StorageDelta const& d) { REQUIRE(!applicator->try_apply(d)); };

    auto make_raw_mem_write = [&](const raw_mem_val& v) -> StorageDelta {
        raw_mem_val copy = v;
        return make_raw_memory_write(std::move(copy));
    };

    auto val_expect_mem_value = [&](raw_mem_val v) {
        REQUIRE(applicator->get());
        REQUIRE(applicator->get()->body.raw_memory_storage().data == v);
    };

    auto val_expect_nullopt = [&]() { REQUIRE(!applicator->get()); };

    raw_mem_val val1 = { 0x01, 0x02, 0x03, 0x04 };
    raw_mem_val val2 = { 0x01, 0x03, 0x05, 0x07 };

    SECTION("no base obj")
    {
        applicator = std::make_unique<ProxyApplicator>(std::nullopt);

        SECTION("one delete last")
        {
            check_valid(make_delete_last());

            val_expect_nullopt();
        }

        SECTION("one write")
        {
            check_valid(make_raw_mem_write(val1));

            val_expect_mem_value(val1);
        }
        SECTION("two write different")
        {
            check_valid(make_raw_mem_write(val1));

            val_expect_mem_value(val1);

            check_valid(make_raw_mem_write(val2));

            val_expect_mem_value(val2);
        }

        SECTION("two write same")
        {
            check_valid(make_raw_mem_write(val1));

            val_expect_mem_value(val1);

            check_valid(make_raw_mem_write(val1));

            val_expect_mem_value(val1);
        }

        SECTION("write and delete")
        {

            check_valid(make_raw_mem_write(val1));
            val_expect_mem_value(val1);

            check_valid(make_delete_last());
            val_expect_nullopt();
        }
        SECTION("write and delete swap order")
        {
            // different from regular typeclass rules,
            // proxyapplicator blocks everything after a delete.
            check_valid(make_delete_last());
            check_invalid(make_raw_mem_write(val1));

            val_expect_nullopt();
        }
    }
    SECTION("populate with something")
    {
        StorageObject obj;
        obj.body.type(RAW_MEMORY);
        obj.body.raw_memory_storage().data = val1;

        applicator = std::make_unique<ProxyApplicator>(obj);

        SECTION("write one same")
        {
            check_valid(make_raw_mem_write(val1));

            val_expect_mem_value(val1);
        }

        SECTION("write one diff")
        {
            check_valid(make_raw_mem_write(val2));

            val_expect_mem_value(val2);
        }

        SECTION("one delete last")
        {
            check_valid(make_delete_last());
            val_expect_nullopt();
        }

        SECTION("write post delete")
        {
            check_valid(make_delete_last());
            val_expect_nullopt();

            check_invalid(make_raw_mem_write(val1));
            val_expect_nullopt();
        }

        SECTION("two writes of same (different from init) value")
        {
            check_valid(make_raw_mem_write(val2));
            check_valid(make_raw_mem_write(val2));

            val_expect_mem_value(val2);
        }
        SECTION("two writes of diff values")
        {
            check_valid(make_raw_mem_write(val2));
            check_valid(make_raw_mem_write(val1));
            val_expect_mem_value(val1);
        }
    }
}

TEST_CASE("nonnegative int64 only", "[mutator]")
{

    std::unique_ptr<ProxyApplicator> applicator;

    auto check_valid
        = [&](StorageDelta const& d) { REQUIRE(applicator->try_apply(d)); };

    auto check_invalid
        = [&](StorageDelta const& d) { REQUIRE(!applicator->try_apply(d)); };

    auto val_expect_int64 = [&](int64_t v) {
        REQUIRE(applicator->get());
        REQUIRE(applicator->get()->body.nonnegative_int64() == v);
    };

    auto val_expect_nullopt = [&]() { REQUIRE(!applicator->get()); };

    auto make_set_add_delta = [](int64_t set_value, int64_t delta_value) {
        return make_nonnegative_int64_set_add(set_value, delta_value);
    };

    SECTION("no base obj")
    {
        applicator = std::make_unique<ProxyApplicator>(std::nullopt);
        SECTION("strict negative")
        {
            check_invalid(make_set_add_delta(0, -1));
            val_expect_nullopt();
        }
        SECTION("pos ok")
        {
            check_valid(make_set_add_delta(0, 1));
            val_expect_int64(1);
        }
        SECTION("pos sub with set ok")
        {
            check_valid(make_set_add_delta(10, -5));
            val_expect_int64(5);
        }
        SECTION("neg add with set ok")
        {
            check_valid(make_set_add_delta(-10, 5));
            val_expect_int64(-5);
        }
        SECTION("neg add with neg bad")
        {
            check_invalid(make_set_add_delta(-10, -5));
            val_expect_nullopt();
        }

        SECTION("conflicting sets bad")
        {
            check_valid(make_set_add_delta(10, -5));
            check_invalid(make_set_add_delta(20, -5));

            val_expect_int64(5);

            check_invalid(make_set_add_delta(10, -6));

            val_expect_int64(5);
        }

        SECTION("conflicting subs bad")
        {
            check_valid(make_set_add_delta(10, -5));
            check_invalid(make_set_add_delta(10, -6));

            val_expect_int64(5);
        }

        SECTION("invalid single delta")
        {
            check_invalid(make_set_add_delta(10, -11));

            val_expect_nullopt();
        }

        SECTION("check overflow multi subs")
        {
            check_valid(make_set_add_delta(INT64_MAX, INT64_MIN + 10));
            check_invalid(make_set_add_delta(INT64_MAX, INT64_MIN + 10));

            val_expect_int64(9);
        }
        SECTION("check pos overflow")
        {
            check_valid(make_set_add_delta(INT64_MAX, 1));
            check_valid(make_set_add_delta(INT64_MAX, INT64_MAX));
            check_valid(make_set_add_delta(INT64_MAX, INT64_MAX));

            val_expect_int64(INT64_MAX);

            check_valid(make_set_add_delta(INT64_MAX, INT64_MIN + 10));
            // delta is INT64_MAX + 9

            val_expect_int64(INT64_MAX);

            check_valid(make_set_add_delta(INT64_MAX, -10));

            val_expect_int64(INT64_MAX - 1);
        }
        SECTION("check neg overflow")
        {
            check_invalid(make_set_add_delta(INT64_MIN, -1));
            check_valid(make_set_add_delta(INT64_MIN, 1));
        }

        SECTION("check neg overflow across multiple deltas")
        {
            check_valid(make_set_add_delta(INT64_MAX, INT64_MIN + 1));
            check_invalid(make_set_add_delta(INT64_MAX, INT64_MIN));

            val_expect_int64(0);
        }

        SECTION("check neg overflow across multiple deltas 2")
        {
            check_valid(make_set_add_delta(INT64_MIN, 0));
            check_invalid(make_set_add_delta(INT64_MIN, -200));

            val_expect_int64(INT64_MIN);
        }
    }
}

TEST_CASE("hash set only", "[mutator]")
{
    std::unique_ptr<ProxyApplicator> applicator;

    auto check_valid
        = [&](StorageDelta const& d) { REQUIRE(applicator->try_apply(d)); };

    auto check_invalid
        = [&](StorageDelta const& d) { REQUIRE(!applicator->try_apply(d)); };

    auto val_expect_hash = [&](Hash const& h) {
        REQUIRE(applicator->get());
        auto hs = applicator->get()->body.hash_set();

        auto find_hash = [&]() {
            for (auto const& h1 : hs.hashes) {
                if (h1 == h) {
                    return true;
                }
            }
            return false;
        };

        REQUIRE(find_hash());
    };

    auto val_nexpect_hash = [&](Hash const& h) {
        REQUIRE(applicator->get());
        auto hs = applicator->get()->body.hash_set();

        auto find_hash = [&]() {
            for (auto const& h1 : hs.hashes) {
                if (h1 == h) {
                    return true;
                }
            }
            return false;
        };

        REQUIRE(!find_hash());
    };

    auto val_expect_nullopt = [&]() { REQUIRE(!applicator->get()); };

    auto make_hash = [](uint64_t i) { return hash_xdr<uint64_t>(i); };

    SECTION("no base obj")
    {
        applicator = std::make_unique<ProxyApplicator>(std::nullopt);

        SECTION("insert one")
        {
            check_valid(make_hash_set_insert(make_hash(0)));
            val_expect_hash(make_hash(0));
        }
        SECTION("insert several")
        {
            check_valid(make_hash_set_insert(make_hash(0)));
            check_valid(make_hash_set_insert(make_hash(1)));
            check_valid(make_hash_set_insert(make_hash(2)));
            check_valid(make_hash_set_insert(make_hash(3)));
            check_valid(make_hash_set_insert(make_hash(4)));

            check_invalid(make_hash_set_insert(make_hash(0)));
            check_invalid(make_hash_set_insert(make_hash(4)));

            val_expect_hash(make_hash(0));
        }
    }
    SECTION("tiny limit")
    {
        StorageObject obj;
        obj.body.type(ObjectType::HASH_SET);
        obj.body.hash_set().max_size = 1;

        applicator = std::make_unique<ProxyApplicator>(obj);

        SECTION("insert one")
        {
            check_valid(make_hash_set_insert(make_hash(0)));
            val_expect_hash(make_hash(0));
        }
        SECTION("insert two")
        {
            check_valid(make_hash_set_insert(make_hash(0)));
            check_invalid(make_hash_set_insert(make_hash(1)));
            val_expect_hash(make_hash(0));
            val_nexpect_hash(make_hash(1));
        }

        SECTION("raise limit")
        {
            check_valid(make_hash_set_increase_limit(64));
            check_valid(make_hash_set_increase_limit(64));

            auto res = applicator->get_deltas();

            REQUIRE(res.size() == 1);
            REQUIRE(res[0].type() == DeltaType::HASH_SET_INCREASE_LIMIT);
            REQUIRE(res[0].limit_increase() == 128);

            check_valid(make_hash_set_insert(make_hash(0)));
            check_invalid(make_hash_set_insert(make_hash(1)));
        }

        SECTION("clear doesn't take effect until post")
        {
            check_valid(make_hash_set_insert(make_hash(0)));
            check_valid(make_hash_set_clear());

            check_invalid(make_hash_set_insert(make_hash(0)));

            val_expect_hash(make_hash(0));

            auto res = applicator->get_deltas();

            auto find_clear = [&]() {
                for (auto const& d : res) {
                    if (d.type() == DeltaType::HASH_SET_CLEAR) {
                        return true;
                    }
                }
                return false;
            };
            REQUIRE(find_clear());
        }
    }
}

} // namespace scs
