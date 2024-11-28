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

#include "hash_set/utils.h"
#include "xdr/storage.h"

#include <xdrpp/types.h>

#include "crypto/hash.h"


namespace scs
{

using xdr::operator==;


TEST(HashsetUtilsTests, HashsetClearNoElts)
{
	HashSet hs;

	normalize_hashset(hs);

	clear_hashset(hs, 0);

	EXPECT_EQ(hs.hashes.size(), 0);

	clear_hashset(hs, 10);
	EXPECT_EQ(hs.hashes.size(), 0);
}

TEST(HashsetUtilsTests, NormalizeHashset)
{
	HashSet hs1, hs2;

	auto add1 = [&] (uint64_t threshold)
	{
		hs1.hashes.push_back(HashSetEntry(hash_xdr<uint64_t>(threshold), threshold));
	};

	auto add2 = [&] (uint64_t threshold)
	{
		hs2.hashes.push_back(HashSetEntry(hash_xdr<uint64_t>(threshold), threshold));
	};

	add1(0);
	add1(1);
	add1(100);
	add1(5);
	add1(17);

	add2(100);
	add2(5);
	add2(17);
	add2(1);
	add2(0);

	EXPECT_FALSE(hs1 == hs2);

	normalize_hashset(hs1);
	normalize_hashset(hs2);

	EXPECT_TRUE(hs1 == hs2);
}

TEST(HashsetUtilsTests, HashsetClearEvenNumElements)
{
	HashSet hs;

	auto add = [&] (uint64_t threshold)
	{
		hs.hashes.push_back(HashSetEntry(hash_xdr<uint64_t>(threshold), threshold));
	};

	add(0);
	add(1);
	add(10);
	add(11);

	normalize_hashset(hs);

	clear_hashset(hs, 0);

	EXPECT_EQ(hs.hashes.size(), 3);

	clear_hashset(hs, 5);

	EXPECT_EQ(hs.hashes.size(), 2);

	clear_hashset(hs, 10);

	EXPECT_EQ(hs.hashes.size(), 1);

	clear_hashset(hs, 11);
	EXPECT_EQ(hs.hashes.size(), 0);

	add(0);
	add(1);
	add(10);
	add(11);

	clear_hashset(hs, UINT64_MAX);
	EXPECT_EQ(hs.hashes.size(), 0);
		
}

TEST(HashsetUtilsTests, HashsetClearOddNumElements)
{
	HashSet hs;

	auto add = [&] (uint64_t threshold)
	{
		hs.hashes.push_back(HashSetEntry(hash_xdr<uint64_t>(threshold), threshold));
	};

	add(0);
	add(1);
	add(5);
	add(10);
	add(11);

	normalize_hashset(hs);

	clear_hashset(hs, 0);

	EXPECT_EQ(hs.hashes.size(), 4);

	clear_hashset(hs, 5);

	EXPECT_EQ(hs.hashes.size(), 2);

	clear_hashset(hs, 10);

	EXPECT_EQ(hs.hashes.size(), 1);

	clear_hashset(hs, 11);
	EXPECT_EQ(hs.hashes.size(), 0);

	add(0);
	add(1);
	add(5);
	add(10);
	add(11);

	clear_hashset(hs, UINT64_MAX);
	EXPECT_EQ(hs.hashes.size(), 0);
}

}