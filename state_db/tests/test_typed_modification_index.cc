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

#include <catch2/catch_test_macros.hpp>

#include "crypto/hash.h"

#include "object/make_delta.h"

#include "state_db/typed_modification_index.h"

namespace scs {

namespace test {
AddressAndKey
indextest_make_addrkey(uint64_t v)
{
    AddressAndKey a;
    auto h1 = hash_xdr<uint64_t>(v);
    auto h2 = hash_xdr<uint64_t>(v + 1);
    std::memcpy(a.data(), h1.data(), h1.size());
    std::memcpy(a.data() + h1.size(), h2.data(), h2.size());
    return a;
}
} // namespace test

TEST_CASE("test key format delete last", "[index]")
{
    AddressAndKey expect_addr = test::indextest_make_addrkey(0);
    auto expect_hash = hash_xdr<uint64_t>(100);

    auto res = make_index_key(expect_addr, make_delete_last(), expect_hash);

    auto bytes = res.get_bytes_array();

    std::vector<uint8_t> expect_bytes;
    expect_bytes.insert(
        expect_bytes.end(), expect_addr.begin(), expect_addr.end());

    // delta_type
    expect_bytes.push_back(0);

    // delta value
    expect_bytes.resize(expect_bytes.size() + 40, 0);

    expect_bytes.insert(
        expect_bytes.end(), expect_hash.begin(), expect_hash.end());

    REQUIRE(bytes.size() == expect_bytes.size());

    REQUIRE(std::memcmp(bytes.data(), expect_bytes.data(), expect_bytes.size())
            == 0);
}

TEST_CASE("test key format raw mem", "[index]")
{
    AddressAndKey expect_addr = test::indextest_make_addrkey(1);
    auto expect_hash = hash_xdr<uint64_t>(101);

    const xdr::opaque_vec<RAW_MEMORY_MAX_LEN> val = { 0x00, 0x01, 0x02, 0x03 };

    auto res = make_index_key(
        expect_addr,
        make_raw_memory_write(xdr::opaque_vec<RAW_MEMORY_MAX_LEN>(val)),
        expect_hash);

    auto bytes = res.get_bytes_array();

    std::vector<uint8_t> expect_bytes;
    expect_bytes.insert(
        expect_bytes.end(), expect_addr.begin(), expect_addr.end());

    // delta_type
    expect_bytes.push_back(1);

    Hash h;
    hash_raw(val.data(), val.size(), h.data());

    expect_bytes.insert(expect_bytes.end(), h.begin(), h.end());

    // delta value
    expect_bytes.resize(expect_bytes.size() + 8, 0);

    expect_bytes.insert(
        expect_bytes.end(), expect_hash.begin(), expect_hash.end());

    REQUIRE(bytes.size() == expect_bytes.size());

    REQUIRE(std::memcmp(bytes.data(), expect_bytes.data(), expect_bytes.size())
            == 0);
}

TEST_CASE("test key format asset pos", "[index]")
{
    AddressAndKey expect_addr = test::indextest_make_addrkey(2);
    auto expect_hash = hash_xdr<uint64_t>(102);

    auto res = make_index_key(expect_addr, make_asset_add(1), expect_hash);

    auto bytes = res.get_bytes_array();

    std::vector<uint8_t> expect_bytes;
    expect_bytes.insert(
        expect_bytes.end(), expect_addr.begin(), expect_addr.end());

    // delta_type
    expect_bytes.push_back(9);

    utils::append_unsigned_big_endian(expect_bytes, static_cast<uint64_t>(1));

    expect_bytes.resize(expect_bytes.size() + 32, 0);

    expect_bytes.insert(
        expect_bytes.end(), expect_hash.begin(), expect_hash.end());

    REQUIRE(bytes.size() == expect_bytes.size());

    REQUIRE(std::memcmp(bytes.data(), expect_bytes.data(), expect_bytes.size())
            == 0);
}

TEST_CASE("test key format asset neg", "[index]")
{
    AddressAndKey expect_addr = test::indextest_make_addrkey(2);
    auto expect_hash = hash_xdr<uint64_t>(102);

    auto res
        = make_index_key(expect_addr, make_asset_add(INT64_MIN), expect_hash);

    auto bytes = res.get_bytes_array();

    std::vector<uint8_t> expect_bytes;
    expect_bytes.insert(
        expect_bytes.end(), expect_addr.begin(), expect_addr.end());

    // delta_type
    expect_bytes.push_back(9);

    utils::append_unsigned_big_endian(expect_bytes,
                                      static_cast<uint64_t>(INT64_MIN));

    expect_bytes.resize(expect_bytes.size() + 32, 0);

    expect_bytes.insert(
        expect_bytes.end(), expect_hash.begin(), expect_hash.end());

    REQUIRE(bytes.size() == expect_bytes.size());

    REQUIRE(std::memcmp(bytes.data(), expect_bytes.data(), expect_bytes.size())
            == 0);
}

} // namespace scs
