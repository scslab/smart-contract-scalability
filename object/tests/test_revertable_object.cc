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
        REQUIRE(object.get_committed_object()->nonnegative_int64() == 150);
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
        REQUIRE(object.get_committed_object()->nonnegative_int64() == 150);
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
        REQUIRE(object.get_committed_object()->nonnegative_int64() == 150);
    }

    SECTION("conflicting mem after rewind")
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

        {
            auto res = object.try_add_delta(mem1);

            REQUIRE(!res);
        }

        REQUIRE(!object.get_committed_object());
        object.commit_round();
        REQUIRE(object.get_committed_object());
        REQUIRE(object.get_committed_object()->raw_memory_storage().data
                == val2);
    }
}

} // namespace scs
