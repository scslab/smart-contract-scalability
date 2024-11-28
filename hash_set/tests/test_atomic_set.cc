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

#include "crypto/hash.h"
#include "hash_set/atomic_set.h"

#include "object/comparators.h"

#include "xdr/storage.h"

#include <algorithm>

namespace scs {


TEST(AtomicHashsetTests, DistinctInserts)
{
    AtomicSet set(10);

    auto good_insert
        = [&](uint64_t i) { EXPECT_TRUE(set.try_insert(HashSetEntry(hash_xdr(i), 0))); };

    good_insert(0);
    good_insert(1);
    good_insert(2);
}

TEST(AtomicHashsetTests, SameInserts)
{
    AtomicSet set(10);

    auto good_insert
        = [&](uint64_t i) { EXPECT_TRUE(set.try_insert(HashSetEntry(hash_xdr(i), 0))); };

    auto bad_insert
        = [&](uint64_t i) { EXPECT_TRUE(!set.try_insert(HashSetEntry(hash_xdr(i), 0))); };

    good_insert(0);
    good_insert(1);
    bad_insert(1);
    bad_insert(0);
    good_insert(2);
}

TEST(AtomicHashsetTests, InsertsTooSmall)
{
    AtomicSet set(1);

    auto good_insert
        = [&](uint64_t i) { EXPECT_TRUE(set.try_insert(HashSetEntry(hash_xdr(i), 0))); };

    auto bad_insert
        = [&](uint64_t i) { EXPECT_TRUE(!set.try_insert(HashSetEntry(hash_xdr(i), 0))); };

    auto insert_several = [&](uint64_t start, uint32_t count) -> bool {
        for (auto i = 0u; i < count; i++) {
            if (!set.try_insert(HashSetEntry(hash_xdr(start + i), 0))) {
                return false;
            }
        }
        return true;
    };

    good_insert(0);
    bad_insert(1); // size limit overflow

    // resize clears the set
    set.resize(5);

    good_insert(0);
    good_insert(1);
    good_insert(2);
    good_insert(3);
    good_insert(4);

    bad_insert(5);

    EXPECT_FALSE(insert_several(10, 5));
}

TEST(AtomicHashsetTests, ResizeToMax)
{
    AtomicSet set(1);

    set.resize(UINT16_MAX);

    auto good_insert
        = [&](uint64_t i) { EXPECT_TRUE(set.try_insert(HashSetEntry(hash_xdr(i), 0))); };

    for (uint64_t i = 0; i < UINT16_MAX; i++)
    {
        good_insert(i);
    }
}

TEST(AtomicHashsetTests, InsertAndDelete)
{
    AtomicSet set(10);

    auto good_insert
        = [&](uint64_t i) { EXPECT_TRUE(set.try_insert(HashSetEntry(hash_xdr(i), 0))); };

    auto bad_insert
        = [&](uint64_t i) { EXPECT_TRUE(!set.try_insert(HashSetEntry(hash_xdr(i), 0))); };

    auto good_erase = [&](uint64_t i) { set.erase(HashSetEntry(hash_xdr(i), 0)); };

    // erase works correctly
    good_insert(1);
    good_insert(2);
    good_insert(3);

    good_erase(2);
    good_erase(1);
    good_erase(3);

    good_insert(1);
    good_insert(2);
    good_insert(3);

    bad_insert(1);
    bad_insert(2);
    bad_insert(3);

    auto res = set.get_hashes();

    ASSERT_EQ(res.size(), 3);
    std::sort(res.begin(), res.end());
    // experimentally determined ordering
    ASSERT_TRUE(res[0].hash == hash_xdr<uint64_t>(1));
    ASSERT_TRUE(res[1].hash == hash_xdr<uint64_t>(3));
    ASSERT_TRUE(res[2].hash == hash_xdr<uint64_t>(2));
}

} // namespace scs