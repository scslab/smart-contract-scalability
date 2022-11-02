#include <catch2/catch_test_macros.hpp>

#include "object/make_delta.h"

#include "object/revertable_object.h"

#include "threadlocal/threadlocal_context.h"

#include "xdr/storage_delta.h"

#include "crypto/hash.h"

using xdr::operator==;

namespace scs {

using raw_mem_val = xdr::opaque_vec<RAW_MEMORY_MAX_LEN>;

const static raw_mem_val val1 = { 0x01, 0x02, 0x03, 0x04 };
const static raw_mem_val val2 = { 0x01, 0x03, 0x05, 0x07 };

TEST_CASE("revert object from empty", "[object]")
{
    RevertableObject object;

    SECTION("nothing")
    {
        object.commit_round();
        REQUIRE(!object.get_committed_object());
    }

    SECTION("one set nnint64 reverted")
    {
        auto set_add = make_nonnegative_int64_set_add(100, 50);
        {
            auto res = object.try_add_delta(set_add);

            REQUIRE(!!res);
        }

        object.commit_round();
        REQUIRE(!object.get_committed_object());
    }

    SECTION("one set nnint64 committed")
    {
        auto set_add = make_nonnegative_int64_set_add(100, 50);
        {
            auto res = object.try_add_delta(set_add);

            REQUIRE(!!res);

            res->commit();
        }

        REQUIRE(!object.get_committed_object());
        object.commit_round();
        REQUIRE(object.get_committed_object());
        REQUIRE(object.get_committed_object()->body.nonnegative_int64() == 150);
    }

    SECTION("conflicting writes int64")
    {
        auto set_add = make_nonnegative_int64_set_add(100, 50);
        auto set_add2 = make_nonnegative_int64_set_add(101, 50);
        {
            auto res = object.try_add_delta(set_add);

            REQUIRE(!object.try_add_delta(set_add2));

            REQUIRE(!!res);

            res->commit();
        }

        REQUIRE(!object.get_committed_object());
        object.commit_round();
        REQUIRE(object.get_committed_object());
        REQUIRE(object.get_committed_object()->body.nonnegative_int64() == 150);
    }

    SECTION("conflicting writes type")
    {
        auto set_add = make_nonnegative_int64_set_add(100, 50);
        auto mem = make_raw_memory_write(raw_mem_val(val1));
        {
            auto res = object.try_add_delta(set_add);

            REQUIRE(!object.try_add_delta(mem));

            REQUIRE(!!res);

            res->commit();
        }

        REQUIRE(!object.get_committed_object());
        object.commit_round();
        REQUIRE(object.get_committed_object());
        REQUIRE(object.get_committed_object()->body.nonnegative_int64() == 150);
    }

    SECTION("conflicting after rewind")
    {
        auto mem1 = make_raw_memory_write(raw_mem_val(val1));
        auto mem2 = make_raw_memory_write(raw_mem_val(val2));

        {
            auto res = object.try_add_delta(mem1);

            REQUIRE(!!res);
        }
        {
            auto res = object.try_add_delta(mem2);

            REQUIRE(!!res);
            res->commit();
        }

        SECTION("conflict mem")
        {
            {
                auto res = object.try_add_delta(mem1);

                REQUIRE(!res);
            }

            REQUIRE(!object.get_committed_object());
            object.commit_round();
            REQUIRE(object.get_committed_object());
            REQUIRE(
                object.get_committed_object()->body.raw_memory_storage().data
                == val2);
        }

        SECTION("conflict int")
        {
            {
                auto set_add = make_nonnegative_int64_set_add(100, 50);

                auto res = object.try_add_delta(set_add);

                REQUIRE(!res);
            }

            REQUIRE(!object.get_committed_object());
            object.commit_round();
            REQUIRE(object.get_committed_object());
            REQUIRE(
                object.get_committed_object()->body.raw_memory_storage().data
                == val2);
        }
    }

    SECTION("int64 decrease conflicts")
    {
        auto set_add = make_nonnegative_int64_set_add(100, -50);

        {
            auto res = object.try_add_delta(set_add);
            REQUIRE(!!res);

            {
                auto res2 = object.try_add_delta(set_add);
                REQUIRE(!!res2);

                auto res3 = object.try_add_delta(set_add);
                REQUIRE(!res3);
            }

            res->commit();

            auto res3 = object.try_add_delta(set_add);
            REQUIRE(!!res3);
            res3->commit();
        }

        REQUIRE(!object.get_committed_object());
        object.commit_round();
        REQUIRE(object.get_committed_object());
        REQUIRE(object.get_committed_object()->body.nonnegative_int64() == 0);
    }
}

TEST_CASE("revert object from nonempty", "[object]")
{
    StorageObject o;
    o.body.type(ObjectType::NONNEGATIVE_INT64);
    o.body.nonnegative_int64() = 30;

    auto set_add = make_nonnegative_int64_set_add(100, -50);

    RevertableObject object(o);

    SECTION("no deltas")
    {
        REQUIRE(object.get_committed_object());
        REQUIRE(object.get_committed_object()->body.nonnegative_int64() == 30);
    }

    SECTION("mem write conflict")
    {
        auto mem = make_raw_memory_write(raw_mem_val(val1));

        REQUIRE(!object.try_add_delta(mem));
    }

    SECTION("set add conflict")
    {
        {
            auto res = object.try_add_delta(set_add);
            REQUIRE(!!res);
            res->commit();
        }

        SECTION("one write goes through")
        {
            object.commit_round();

            REQUIRE(object.get_committed_object());
            REQUIRE(object.get_committed_object()->body.nonnegative_int64()
                    == 50);
        }
        SECTION("three writes no go")
        {
            {
                auto res1 = object.try_add_delta(set_add);
                REQUIRE(!!res1);

                auto res2 = object.try_add_delta(set_add);
                REQUIRE(!res2);

                res1->commit();
            }

            object.commit_round();
            REQUIRE(object.get_committed_object());
            REQUIRE(object.get_committed_object()->body.nonnegative_int64()
                    == 0);
        }
    }

    SECTION("delete")
    {
        {
            auto del = make_delete_last();
            auto res = object.try_add_delta(del);

            REQUIRE(!!res);

            res->commit();
        }

        REQUIRE(object.get_committed_object());
        object.commit_round();
        REQUIRE(!object.get_committed_object());

        SECTION("try rewriting to mem")
        {
            auto mem = make_raw_memory_write(raw_mem_val(val1));
            {
                auto res = object.try_add_delta(mem);

                REQUIRE(!!res);
                res->commit();
            }

            object.commit_round();

            REQUIRE(object.get_committed_object());

            REQUIRE(
                object.get_committed_object()->body.raw_memory_storage().data
                == val1);
        }

        SECTION("try rewriting to int64")
        {
            auto set_add = make_nonnegative_int64_set_add(-50, 100);
            {
                auto res = object.try_add_delta(set_add);

                REQUIRE(!!res);
                res->commit();
            }

            object.commit_round();

            REQUIRE(object.get_committed_object());

            REQUIRE(object.get_committed_object()->body.nonnegative_int64()
                    == 50);
        }
    }
}

TEST_CASE("hashset from empty", "[object]")
{
    test::DeferredContextClear defer;

    RevertableObject object;

    auto make_insert = [](uint64_t i) {
        return make_hash_set_insert(hash_xdr<uint64_t>(i), 0);
    };

    auto good_add = [&](StorageDelta const& d) {
        auto res = object.try_add_delta(d);
        REQUIRE(!!res);
        res->commit();
    };

    auto revert_good_add = [&](StorageDelta const& d) {
        auto res = object.try_add_delta(d);
        REQUIRE(!!res);
    };

    SECTION("insert & revert")
    {
        {
            auto res = object.try_add_delta(make_insert(0));
            REQUIRE(!!res);
        }

        object.commit_round();
        REQUIRE(!object.get_committed_object());
    }

    SECTION("insert many, keep one")
    {
        {
            auto res = object.try_add_delta(make_insert(0));
            REQUIRE(!!res);
            res->commit();
        }

        {
            auto res = object.try_add_delta(make_insert(1));
            REQUIRE(!!res);
        }

        {
            auto res = object.try_add_delta(make_insert(1));
            REQUIRE(!!res);
            auto res2 = object.try_add_delta(make_insert(1));
            REQUIRE(!res2);
        }

        object.commit_round();
        REQUIRE(object.get_committed_object());
        auto const& hashes
            = object.get_committed_object()->body.hash_set().hashes;

        REQUIRE(hashes.size() == 1);
        REQUIRE(hashes[0].hash == hash_xdr<uint64_t>(0));
    }

    SECTION("raise limit")
    {
        SECTION("overflow")
        {
            auto res = object.try_add_delta(
                make_hash_set_increase_limit(UINT16_MAX));
            REQUIRE(!!res);
            res->commit();

            auto res2 = object.try_add_delta(
                make_hash_set_increase_limit(UINT16_MAX));
            REQUIRE(!!res2);
            res2->commit();

            object.commit_round();
            REQUIRE(object.get_committed_object());

            REQUIRE(object.get_committed_object()->body.hash_set().hashes.size()
                    == 0);
            REQUIRE(object.get_committed_object()->body.hash_set().max_size
                    == MAX_HASH_SET_SIZE);
        }

        SECTION("no overflow, with a revert")
        {
            {
                auto res
                    = object.try_add_delta(make_hash_set_increase_limit(64));
                REQUIRE(!!res);
                res->commit();

                auto res2 = object.try_add_delta(
                    make_hash_set_increase_limit(UINT16_MAX));
                REQUIRE(!!res2);
            }

            object.commit_round();

            REQUIRE(object.get_committed_object());

            REQUIRE(object.get_committed_object()->body.hash_set().hashes.size()
                    == 0);
            REQUIRE(object.get_committed_object()->body.hash_set().max_size
                    == 64 + START_HASH_SET_SIZE);
        }
    }

    SECTION("clear")
    {
        good_add(make_insert(0));
        good_add(make_insert(1));

        SECTION("revert clear")
        {
            revert_good_add(make_hash_set_clear(0));

            object.commit_round();
            REQUIRE(object.get_committed_object());
            REQUIRE(object.get_committed_object()->body.hash_set().hashes.size()
                    == 2);
        }

        SECTION("commit clear")
        {
            good_add(make_hash_set_clear(0));

            object.commit_round();
            REQUIRE(object.get_committed_object());
            REQUIRE(object.get_committed_object()->body.hash_set().hashes.size()
                    == 0);
        }
    }
}

TEST_CASE("hashset from nonempty", "[object]")
{
    test::DeferredContextClear defer;

    StorageObject base_obj;
    base_obj.body.type(ObjectType::HASH_SET);
    base_obj.body.hash_set().max_size = 4;
    base_obj.body.hash_set().hashes.push_back(HashSetEntry(hash_xdr<uint64_t>(0), 0));
    base_obj.body.hash_set().hashes.push_back(HashSetEntry(hash_xdr<uint64_t>(1), 0));

    RevertableObject object(base_obj);

    auto make_insert = [](uint64_t i) {
        return make_hash_set_insert(hash_xdr<uint64_t>(i), 0);
    };

    auto good_add = [&](StorageDelta const& d) {
        auto res = object.try_add_delta(d);
        REQUIRE(!!res);
        res->commit();
    };

    auto bad_add
        = [&](StorageDelta const& d) { REQUIRE(!object.try_add_delta(d)); };

    SECTION("too many insert")
    {
        good_add(make_insert(2));
        good_add(make_insert(3));

        bad_add(make_insert(4));
    }

    SECTION("insert already exist")
    {
        bad_add(make_insert(1));

        good_add(make_insert(2));
        bad_add(make_insert(2));
    }
    SECTION("clear")
    {
        good_add(make_hash_set_clear(0));

        object.commit_round();
        REQUIRE(object.get_committed_object());
        REQUIRE(object.get_committed_object()->body.hash_set().hashes.size()
                == 0);
    }
}

TEST_CASE("hashset from nonempty with thresholds", "[object]")
{

    StorageObject base_obj;
    base_obj.body.type(ObjectType::HASH_SET);
    base_obj.body.hash_set().max_size = 4;
    base_obj.body.hash_set().hashes.push_back(HashSetEntry(hash_xdr<uint64_t>(0), 10));
    base_obj.body.hash_set().hashes.push_back(HashSetEntry(hash_xdr<uint64_t>(1), 20));

    RevertableObject object(base_obj);

    auto make_insert = [](uint64_t i, uint64_t threshold) {
        return make_hash_set_insert(hash_xdr<uint64_t>(i), threshold);
    };

    auto good_add = [&](StorageDelta const& d) {
        auto res = object.try_add_delta(d);
        REQUIRE(!!res);
        res->commit();
    };

    SECTION("new entry, repeated hash")
    {
        good_add(make_insert(0, 15));
        good_add(make_insert(2, 25));

        object.commit_round();

        REQUIRE(object.get_committed_object());

        auto const& hs = object.get_committed_object() -> body.hash_set().hashes;

        REQUIRE(hs.size() == 4);
        REQUIRE(hs[0].hash == hash_xdr<uint64_t>(2));
        REQUIRE(hs[1].hash == hash_xdr<uint64_t>(1));
        REQUIRE(hs[2].hash == hash_xdr<uint64_t>(0));
        REQUIRE(hs[3].hash == hash_xdr<uint64_t>(0));
    }

    SECTION("clear filtering")
    {
        good_add(make_insert(0, 15));
        good_add(make_insert(2, 25));

        good_add(make_hash_set_clear(15));
        good_add(make_hash_set_clear(10));

        object.commit_round();

        REQUIRE(object.get_committed_object());

        auto const& hs = object.get_committed_object() -> body.hash_set().hashes;

        REQUIRE(hs.size() == 2);
        REQUIRE(hs[0].hash == hash_xdr<uint64_t>(2));
        REQUIRE(hs[1].hash == hash_xdr<uint64_t>(1));
    }
}


} // namespace scs
