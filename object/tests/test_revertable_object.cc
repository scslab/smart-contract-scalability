/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include "object/make_delta.h"

#include "object/revertable_object.h"

#include "threadlocal/threadlocal_context.h"

#include "transaction_context/execution_context.h"

#include "xdr/storage_delta.h"

#include "crypto/hash.h"

using xdr::operator==;

namespace scs {

using raw_mem_val = xdr::opaque_vec<RAW_MEMORY_MAX_LEN>;

const static raw_mem_val val1 = { 0x01, 0x02, 0x03, 0x04 };
const static raw_mem_val val2 = { 0x01, 0x03, 0x05, 0x07 };

class RevertableObjectTests : public ::testing::Test {
 protected:
  test::DeferredContextClear defer;
}; 


TEST_F(RevertableObjectTests, RevertFromEmptyNothing)
{
    RevertableObject object;

    EXPECT_FALSE(!!object.get_committed_object());

    object.commit_round();
    
    EXPECT_FALSE(!!object.get_committed_object());
}

TEST_F(RevertableObjectTests, RevertOneNNInt64Set)
{
    RevertableObject object;

    auto set_add = make_nonnegative_int64_set_add(100, 50);
    {
        auto res = object.try_add_delta(set_add);

        ASSERT_TRUE(!!res);
    }

    object.commit_round();
    EXPECT_FALSE(!!object.get_committed_object());
}

TEST_F(RevertableObjectTests, CommitOneNNInt64Set)
{
    RevertableObject object;

    auto set_add = make_nonnegative_int64_set_add(100, 50);
    {
        auto res = object.try_add_delta(set_add);

        ASSERT_TRUE(!!res);

        res->commit();
    }

    EXPECT_FALSE(!!object.get_committed_object());
    object.commit_round();
    ASSERT_TRUE(!!object.get_committed_object());
    EXPECT_EQ(object.get_committed_object()->body.nonnegative_int64(), 150);
}

TEST_F(RevertableObjectTests, ConflictingNNInt64Writes)
{
    RevertableObject object;
    auto set_add = make_nonnegative_int64_set_add(100, 50);
    auto set_add2 = make_nonnegative_int64_set_add(101, 50);
    {
        auto res = object.try_add_delta(set_add);

        EXPECT_FALSE(!!object.try_add_delta(set_add2));

        ASSERT_TRUE(!!res);

        res->commit();
    }

    EXPECT_FALSE(!!object.get_committed_object());
    object.commit_round();
    ASSERT_TRUE(!!object.get_committed_object());
    EXPECT_EQ(object.get_committed_object()->body.nonnegative_int64(), 150);
}

TEST_F(RevertableObjectTests, ConflictingWritesType)
{
    RevertableObject object;

    auto set_add = make_nonnegative_int64_set_add(100, 50);
    auto mem = make_raw_memory_write(raw_mem_val(val1));
    {
        auto res = object.try_add_delta(set_add);

        EXPECT_FALSE(!!object.try_add_delta(mem));

        ASSERT_TRUE(!!res);

        res->commit();
    }

    EXPECT_FALSE(!!object.get_committed_object());
    object.commit_round();
    ASSERT_TRUE(!!object.get_committed_object());
    EXPECT_EQ(object.get_committed_object()->body.nonnegative_int64(), 150);
}

TEST_F(RevertableObjectTests, ConflictingAfterRewind)
{
    RevertableObject object;

    auto mem1 = make_raw_memory_write(raw_mem_val(val1));
    auto mem2 = make_raw_memory_write(raw_mem_val(val2));
    auto set_add = make_nonnegative_int64_set_add(100, 50);

    {
        auto res = object.try_add_delta(mem1);

        ASSERT_TRUE(!!res);
    }

    {
        auto res = object.try_add_delta(set_add);

        ASSERT_TRUE(!!res);
    }

    {
        auto res = object.try_add_delta(mem2);

        ASSERT_TRUE(!!res);
        res->commit();
    }

    {

        auto res = object.try_add_delta(mem1);

        ASSERT_FALSE(!!res);
    }

    EXPECT_FALSE(!!object.get_committed_object());
    object.commit_round();
    ASSERT_TRUE(!!object.get_committed_object());
    EXPECT_EQ(object.get_committed_object()->body.raw_memory_storage().data, val2);
}

TEST_F(RevertableObjectTests, IntDecreaseConflicts)
{
    RevertableObject object;

    auto set_add = make_nonnegative_int64_set_add(100, -50);

    {
        auto res = object.try_add_delta(set_add);
        ASSERT_TRUE(!!res);

        {
            auto res2 = object.try_add_delta(set_add);
            ASSERT_TRUE(!!res2);

            auto res3 = object.try_add_delta(set_add);
            ASSERT_FALSE(!!res3);
        }

        res->commit();

        auto res3 = object.try_add_delta(set_add);
        ASSERT_TRUE(!!res3);
        res3->commit();
    }

    EXPECT_FALSE(!!object.get_committed_object());
    object.commit_round();
    ASSERT_TRUE(!!object.get_committed_object());
    EXPECT_EQ(object.get_committed_object()->body.nonnegative_int64(), 0);
}

TEST_F(RevertableObjectTests, RevertFromNonempty)
{
    StorageObject o;
    o.body.type(ObjectType::NONNEGATIVE_INT64);
    o.body.nonnegative_int64() = 30;

    auto set_add = make_nonnegative_int64_set_add(110, -50);

    RevertableObject object(o);

    ASSERT_TRUE(!!object.get_committed_object());
    EXPECT_EQ(object.get_committed_object()->body.nonnegative_int64(), 30);

    auto mem = make_raw_memory_write(raw_mem_val(val1));
    EXPECT_FALSE(!!object.try_add_delta(mem));

    {
        auto res = object.try_add_delta(set_add);
        ASSERT_TRUE(!!res);
        res -> commit();
    }

    {
        auto res1 = object.try_add_delta(set_add);
        ASSERT_TRUE(!!res1);

        auto res2 = object.try_add_delta(set_add);
        ASSERT_FALSE(!!res2);

        res1->commit();
    }

    object.commit_round();
    ASSERT_TRUE(!!object.get_committed_object());
    EXPECT_EQ(object.get_committed_object()->body.nonnegative_int64(), 10);
}

TEST_F(RevertableObjectTests, ChangeTypeAfterDelete)
{
    StorageObject o;
    o.body.type(ObjectType::NONNEGATIVE_INT64);
    o.body.nonnegative_int64() = 30;

    auto set_add = make_nonnegative_int64_set_add(110, -50);

    RevertableObject object(o);

    ASSERT_TRUE(!!object.get_committed_object());
    EXPECT_EQ(object.get_committed_object()->body.nonnegative_int64(), 30);

    {
        auto res = object.try_add_delta(make_delete_last());

        ASSERT_TRUE(!!res);

        res->commit();
    }

    ASSERT_TRUE(!!object.get_committed_object());
    object.commit_round();
    ASSERT_FALSE(!!object.get_committed_object());

    {
        auto mem = make_raw_memory_write(raw_mem_val(val1));
        {
            auto res = object.try_add_delta(mem);

            ASSERT_TRUE(!!res);
            res->commit();
        }

        object.commit_round();

        ASSERT_TRUE(object.get_committed_object());

        EXPECT_EQ(
            object.get_committed_object()->body.raw_memory_storage().data, val1);
    }

    // delete again

    {
        auto res = object.try_add_delta(make_delete_last());
        ASSERT_TRUE(!!res);
        res->commit();
    }

    object.commit_round();

    ASSERT_FALSE(!!object.get_committed_object());

    // rewrite to an int64 object
    {
        auto set_add = make_nonnegative_int64_set_add(-50, 100);
        {
            auto res = object.try_add_delta(set_add);

            ASSERT_TRUE(!!res);
            res->commit();
        }

        object.commit_round();

        ASSERT_TRUE(!!object.get_committed_object());

        EXPECT_EQ(object.get_committed_object()->body.nonnegative_int64(), 50);
    }

}

TEST_F(RevertableObjectTests, HashsetFromEmpty)
{
    RevertableObject object;

    auto make_insert = [](uint64_t i) {
        return make_hash_set_insert(hash_xdr<uint64_t>(i), 0);
    };

    auto good_add = [&](StorageDelta const& d) {
        auto res = object.try_add_delta(d);
        ASSERT_TRUE(!!res);
        res->commit();
    };

    auto revert_good_add = [&](StorageDelta const& d) {
        auto res = object.try_add_delta(d);
        ASSERT_TRUE(!!res);
    };

    revert_good_add(make_insert(0));

    object.commit_round();

    ASSERT_FALSE(!!object.get_committed_object());

    //insert many, keep one

    revert_good_add(make_insert(1));

    good_add(make_insert(0));

    revert_good_add(make_insert(2));
    revert_good_add(make_insert(3));
    revert_good_add(make_insert(3));
    revert_good_add(make_insert(2));

    object.commit_round();
    
    ASSERT_TRUE(!!object.get_committed_object());
    auto const& hashes
        = object.get_committed_object()->body.hash_set().hashes;

    ASSERT_EQ(hashes.size(), 1);
    EXPECT_EQ(hashes[0].hash, hash_xdr<uint64_t>(0));

    // try raising hashset limit

    {
        auto res = object.try_add_delta(make_hash_set_increase_limit(64));
        ASSERT_TRUE(!!res);
        res->commit();
    }

    {
        auto res = object.try_add_delta(make_hash_set_increase_limit(UINT16_MAX));
        ASSERT_TRUE(!!res);
    }

    object.commit_round();

    ASSERT_TRUE(!!object.get_committed_object());
    EXPECT_EQ(object.get_committed_object()->body.hash_set().hashes.size(), 1);
    EXPECT_EQ(object.get_committed_object()->body.hash_set().max_size, 64 + START_HASH_SET_SIZE);

    {
        // overflow hashset size increase
         auto res = object.try_add_delta(
            make_hash_set_increase_limit(UINT16_MAX));
        ASSERT_TRUE(!!res);
        res->commit();

        auto res2 = object.try_add_delta(
            make_hash_set_increase_limit(UINT16_MAX));
        ASSERT_TRUE(!!res2);
        res2->commit();
    }

    object.commit_round();

    ASSERT_TRUE(!!object.get_committed_object());
    EXPECT_EQ(object.get_committed_object()->body.hash_set().hashes.size(), 1);
    EXPECT_EQ(object.get_committed_object()->body.hash_set().max_size, MAX_HASH_SET_SIZE);
}

TEST_F(RevertableObjectTests, HashsetClear)
{
    RevertableObject object;

    auto make_insert = [](uint64_t i) {
        return make_hash_set_insert(hash_xdr<uint64_t>(i), 0);
    };

    auto good_add = [&](StorageDelta const& d) {
        auto res = object.try_add_delta(d);
        ASSERT_TRUE(!!res);
        res->commit();
    };

    auto revert_good_add = [&](StorageDelta const& d) {
        auto res = object.try_add_delta(d);
        ASSERT_TRUE(!!res);
    };

    good_add(make_insert(0));
    good_add(make_insert(1));

    revert_good_add(make_hash_set_clear(0));

    object.commit_round();
    ASSERT_TRUE(!!object.get_committed_object());
    EXPECT_EQ(object.get_committed_object()->body.hash_set().hashes.size(), 2);


    good_add(make_hash_set_clear(0));
    object.commit_round();
    
    ASSERT_TRUE(!!object.get_committed_object());
    EXPECT_EQ(object.get_committed_object()->body.hash_set().hashes.size(), 0);
}

TEST_F(RevertableObjectTests, HashsetFromNonempty)
{
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
        ASSERT_TRUE(!!res);
        res->commit();
    };

    auto bad_add
        = [&](StorageDelta const& d) { ASSERT_FALSE(!!object.try_add_delta(d)); };

    good_add(make_insert(2));

     // check already inserted is bad
    bad_add(make_insert(1));
    bad_add(make_insert(2));


    good_add(make_insert(3));
    // over size limit
    bad_add(make_insert(4));


    good_add(make_hash_set_clear(0));

    object.commit_round();
    ASSERT_TRUE(!!object.get_committed_object());
    EXPECT_EQ(object.get_committed_object()->body.hash_set().hashes.size(), 0);
}

TEST_F(RevertableObjectTests, HashsetFromNonemptyWithThresholds)
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
        ASSERT_TRUE(!!res);
        res->commit();
    };

    auto bad_add = [&](StorageDelta const& d) {
        auto res = object.try_add_delta(d);
        ASSERT_FALSE(!!res);
    };

    bad_add(make_insert(0, 15));
    bad_add(make_insert(0, 10));
    bad_add(make_insert(0, 500));

    good_add(make_insert(2, 25));
    good_add(make_insert(3, 9));

    object.commit_round();

    ASSERT_TRUE(!!object.get_committed_object());

    {
        auto const& hs = object.get_committed_object() -> body.hash_set().hashes;

        ASSERT_EQ(hs.size(), 4);
        EXPECT_EQ(hs[0].hash, hash_xdr<uint64_t>(2));
        EXPECT_EQ(hs[1].hash, hash_xdr<uint64_t>(1));
        EXPECT_EQ(hs[2].hash, hash_xdr<uint64_t>(0));
        EXPECT_EQ(hs[3].hash, hash_xdr<uint64_t>(3));
    }

    // try filtering

    good_add(make_hash_set_clear(20));
    good_add(make_hash_set_clear(10));

    object.commit_round();

    ASSERT_TRUE(!!object.get_committed_object());

    {
        auto const& hs = object.get_committed_object() -> body.hash_set().hashes;

        ASSERT_EQ(hs.size(), 1);
        EXPECT_EQ(hs[0].hash, hash_xdr<uint64_t>(2));
    }
}

TEST_F(RevertableObjectTests, HSSizeLimitIncreasesRespected)
{
    RevertableObject object;

    auto make_insert = [](uint64_t i) {
        return make_hash_set_insert(hash_xdr<uint64_t>(i), 0);
    };

    auto r = object.try_add_delta(make_hash_set_increase_limit(64));
    ASSERT_TRUE(!!r);
    r->commit();

    for (size_t i = 0; i < 64; i++)
    {
        auto h = object.try_add_delta(make_insert(i));
        ASSERT_TRUE(!!h);
        h -> commit();
    }

    ASSERT_FALSE(!!object.try_add_delta(make_insert(64)));

    object.commit_round();

    for (size_t i = 64; i < 128; i++)
    {
        auto h = object.try_add_delta(make_insert(i));
        ASSERT_TRUE(!!h);
        h -> commit();
    }
} 

TEST_F(RevertableObjectTests, AssetFromEmpty)
{
    std::unique_ptr<RevertableObject> object = std::make_unique<RevertableObject>();

    auto good_add_and_commit = [&](int64_t d) {
        auto res = object->try_add_delta(make_asset_add(d));
        ASSERT_TRUE(!!res);
        res->commit();
    };

    auto bad_add = [&](int64_t d) {
        auto res = object->try_add_delta(make_asset_add(d));
        ASSERT_FALSE(!!res);
    };

    auto expect_committed = [&](uint64_t value)
    {
        object->commit_round();
        auto o = object->get_committed_object();
        ASSERT_TRUE(!!o);

        ASSERT_EQ(o->body.type(), ObjectType::KNOWN_SUPPLY_ASSET);

        EXPECT_EQ(o->body.asset().amount, value);
    };

    good_add_and_commit(10);
    good_add_and_commit(-10);
    expect_committed(0);

    object = std::make_unique<RevertableObject>();

    bad_add(-1);
    good_add_and_commit(10);
    bad_add(-11);
    good_add_and_commit(-10);
    expect_committed(0);

    object = std::make_unique<RevertableObject>();

    good_add_and_commit(INT64_MAX);
    good_add_and_commit(INT64_MAX);
    good_add_and_commit(1);
    bad_add(1);
    good_add_and_commit(-10);
    good_add_and_commit(5);
    bad_add(6);
    expect_committed(UINT64_MAX-5);

}

TEST_F(RevertableObjectTests, AssetIntermingledCommits)
{
    std::unique_ptr<RevertableObject> object;

    auto good_add = [&](int64_t d)
    {
        auto res = object->try_add_delta(make_asset_add(d));
        EXPECT_TRUE(!!res);
        return res;
    };

    auto bad_add = [&](int64_t d)
    {
        auto res = object->try_add_delta(make_asset_add(d));
        ASSERT_FALSE(!!res);
    };

    auto expect_committed = [&](uint64_t value)
    {
        object->commit_round();
        auto o = object->get_committed_object();
        ASSERT_TRUE(!!o);

        ASSERT_TRUE(o->body.type() == ObjectType::KNOWN_SUPPLY_ASSET);

        ASSERT_TRUE(o->body.asset().amount == value);
    };

    object = std::make_unique<RevertableObject>();
    {
        // can't subtract before add commits
        auto r1 = good_add(10);
        bad_add(-1);
        r1->commit();
        good_add(-1)->commit();

        expect_committed(9);
    }

    object = std::make_unique<RevertableObject>();
    
    {
        // can't add before a subtract commits)
        good_add(INT64_MAX)->commit();
        good_add(2)->commit();

        bad_add(INT64_MAX);

        auto r1 = good_add(-1);

        bad_add(INT64_MAX);

        r1->commit();

        good_add(INT64_MAX)->commit();

        expect_committed(UINT64_MAX);
    }

    object = std::make_unique<RevertableObject>();
    // both ends closed
    {
        auto r1 = good_add(INT64_MAX);
        auto r2 = good_add(INT64_MAX);

        bad_add(3);
        bad_add(-3);
    }

    object = std::make_unique<RevertableObject>();

    // reverts accounted for
    {
        good_add(100)->commit();
        {
            auto r1 = good_add(-100);

            bad_add(-1);
        }
        good_add(-100)->commit();
        expect_committed(0);
    }
}

TEST_F(RevertableObjectTests, AssetFromObject)
{    
    StorageObject base_obj;
    base_obj.body.type(ObjectType::KNOWN_SUPPLY_ASSET);
    base_obj.body.asset().amount = 100;


    std::unique_ptr<RevertableObject> object = std::make_unique<RevertableObject>(base_obj);

    auto good_add = [&](int64_t d)
    {
        auto res = object->try_add_delta(make_asset_add(d));
        EXPECT_TRUE(!!res);
        return res;
    };

    auto bad_add = [&](int64_t d)
    {
        auto res = object->try_add_delta(make_asset_add(d));
        ASSERT_FALSE(!!res);
    };

    auto expect_committed = [&](uint64_t value)
    {
        object->commit_round();
        auto o = object->get_committed_object();
        ASSERT_TRUE(!!o);

        ASSERT_TRUE(o->body.type() == ObjectType::KNOWN_SUPPLY_ASSET);

        ASSERT_TRUE(o->body.asset().amount == value);
    };

    // nothing
    {
        expect_committed(100);
    }

    object = std::make_unique<RevertableObject>(base_obj);

    // one subtract
    {
        auto r1 = good_add(-100);
        bad_add(-1);

        r1->commit();

        expect_committed(0);
    }

    object = std::make_unique<RevertableObject>(base_obj);

    // commit twice
    {
        good_add(100) -> commit();
        expect_committed(200);
        good_add(100) -> commit();
        expect_committed(300);
    }
}

} // namespace scs
