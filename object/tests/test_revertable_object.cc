#include <catch2/catch_test_macros.hpp>

#include "object/make_delta.h"

#include "object/revertable_object.h"

#include "xdr/storage_delta.h"

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
            REQUIRE(object.get_committed_object()->body.raw_memory_storage().data
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
            REQUIRE(object.get_committed_object()->body.raw_memory_storage().data
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
            REQUIRE(object.get_committed_object()->body.nonnegative_int64() == 50);
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
            REQUIRE(object.get_committed_object()->body.nonnegative_int64() == 0);
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

            REQUIRE(object.get_committed_object()->body.raw_memory_storage().data
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

            REQUIRE(object.get_committed_object()->body.nonnegative_int64() == 50);
        }
    }
}

} // namespace scs
