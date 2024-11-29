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

#include "storage_proxy/proxy_applicator.h"

#include "crypto/hash.h"

#include "xdr/storage_delta.h"
#include "xdr/transaction.h"

namespace scs {

using xdr::operator==;

using raw_mem_val = xdr::opaque_vec<RAW_MEMORY_MAX_LEN>;

const static raw_mem_val val1 = { 0x01, 0x02, 0x03, 0x04 };
const static raw_mem_val val2 = { 0x01, 0x03, 0x05, 0x07 };

class ProxyApplicatorTests : public ::testing::Test {
 protected:

 std::unique_ptr<ProxyApplicator> applicator;

 void check_valid(StorageDelta const& d) { 
    if (!applicator->try_apply(d)) {
        throw std::runtime_error("check valid failed");
    }
  }

  void check_invalid(StorageDelta const& d) { 
    if (applicator->try_apply(d)) {
        throw std::runtime_error("check invalid failed");
    }
  }
  using raw_mem_val = xdr::opaque_vec<RAW_MEMORY_MAX_LEN>;

  raw_mem_val val1 = { 0x01, 0x02, 0x03, 0x04 };
  raw_mem_val val2 = { 0x01, 0x03, 0x05, 0x07 };

  static StorageDelta make_raw_mem_write(const raw_mem_val& v) {
    raw_mem_val copy = v;
    return make_raw_memory_write(std::move(copy));
  };

  static StorageDelta make_set_add_delta(int64_t set_value, int64_t delta_value) {
    return make_nonnegative_int64_set_add(set_value, delta_value);
  }

  void val_expect_mem_value (raw_mem_val const& v) {
    if (!applicator || !applicator->get().has_value()) {
        throw std::runtime_error("failed to get value");
    }
    if (applicator -> get()->body.raw_memory_storage().data != v) {
        throw std::runtime_error("raw mem val mismatch");
    }
  }

  void val_expect_int64(int64_t v) {
    if (!applicator || !applicator->get().has_value()) {
        throw std::runtime_error("failed to get value");
    }
    if (applicator -> get()->body.nonnegative_int64() != v) {
        throw std::runtime_error("int64 mismatch");
    }
  }

  void val_expect_nullopt() { 
    if (!applicator || applicator->get().has_value()) {
        throw std::runtime_error("val_expect_nullopt fail");
    }
  }

  void val_expect_hash(Hash const& h) {
    if (!applicator || !applicator->get().has_value()) {
       throw std::runtime_error("val_expect_hash fail");
    }

    auto const& hs = applicator->get()->body.hash_set();

    auto find_hash = [&]() {
        for (auto const& h1 : hs.hashes) {
            if (h1.hash == h) {
                return true;
            }
        }
        return false;
    };

    if (!find_hash()) {
        throw std::runtime_error("hash not found");
    }
  }

  void val_nexpect_hash(Hash const& h) {
    if (!applicator || !applicator->get().has_value()) {
       throw std::runtime_error("val_expect_hash fail");
    }

    auto const& hs = applicator->get()->body.hash_set();

    auto find_hash = [&]() {
        for (auto const& h1 : hs.hashes) {
            if (h1.hash == h) {
                return true;
            }
        }
        return false;
    };

    if (find_hash()) {
        throw std::runtime_error("hash found");
    }
  }

  void val_expect_amount(uint64_t amount) {
    if (!applicator || !applicator->get().has_value()) {
        throw std::runtime_error("val_expect_amount fail");
    }
    if (applicator->get()->body.asset().amount != amount) {
        throw std::runtime_error("unexpected amount");
    }
  }

  // expect an asset delta
  void expect_delta(int64_t delta)
    {
        if (applicator -> get_deltas().size() != 1) {
            throw std::runtime_error("unexpected deltas size");
        }
        if (applicator -> get_deltas().at(0).asset_delta() != delta) {
            throw std::runtime_error("unexpected delta amount");
        }
    };
}; 

TEST_F(ProxyApplicatorTests, RawMemoryOnly)
{
    // no base object section

    // no write
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_delete_last());
    val_expect_nullopt();

    // one write
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_raw_mem_write(val1));
    val_expect_mem_value(val1);

    // two write different
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_raw_mem_write(val1));
    check_valid(make_raw_mem_write(val2));
    val_expect_mem_value(val2);

    // two write same
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_raw_mem_write(val1));
    val_expect_mem_value(val1);
    check_valid(make_raw_mem_write(val1));
    val_expect_mem_value(val1);

    // write and delete
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_raw_mem_write(val1));
    val_expect_mem_value(val1);
    check_valid(make_delete_last());
    val_expect_nullopt();

    // write and delete, swapped order
    // different from regular typeclass rules,
    // proxyapplicator blocks everything after a delete.
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_delete_last());
    check_invalid(make_raw_mem_write(val1));
    val_expect_nullopt();

    // populated with something section
    StorageObject obj;
    obj.body.type(RAW_MEMORY);
    obj.body.raw_memory_storage().data = val1;

    // write one same
    applicator = std::make_unique<ProxyApplicator>(obj);
    val_expect_mem_value(val1);
    check_valid(make_raw_mem_write(val1));
    val_expect_mem_value(val1);

    // write one diff
    applicator = std::make_unique<ProxyApplicator>(obj);
    val_expect_mem_value(val1);
    check_valid(make_raw_mem_write(val2));
    val_expect_mem_value(val2);

    // one delete last
    applicator = std::make_unique<ProxyApplicator>(obj);
    check_valid(make_delete_last());
    val_expect_nullopt();

    // write post delete
    applicator = std::make_unique<ProxyApplicator>(obj);
    check_valid(make_delete_last());
    val_expect_nullopt();
    check_invalid(make_raw_mem_write(val1));
    val_expect_nullopt();

    // two writes of same (different from init) value
    applicator = std::make_unique<ProxyApplicator>(obj);
    check_valid(make_raw_mem_write(val2));
    check_valid(make_raw_mem_write(val2));
    val_expect_mem_value(val2);
        
    // two writes of diff values
    applicator = std::make_unique<ProxyApplicator>(obj);
    check_valid(make_raw_mem_write(val2));
    check_valid(make_raw_mem_write(val1));
    val_expect_mem_value(val1);
}

TEST_F(ProxyApplicatorTests, NNInt64Only)
{
    // strict negative
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_invalid(make_set_add_delta(0, -1));
    val_expect_nullopt();

    // pos ok
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_set_add_delta(0, 1));
    val_expect_int64(1);
        
    // pos sub with set ok
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_set_add_delta(10, -5));
    val_expect_int64(5);
        
    // neg add with set ok
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_set_add_delta(-10, 5));
    val_expect_int64(-5);
        
    // neg add with neg bad
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_invalid(make_set_add_delta(-10, -5));
    val_expect_nullopt();
        
    // conflicting sets bad
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_set_add_delta(10, -5));
    check_invalid(make_set_add_delta(20, -5));
    val_expect_int64(5);
    check_invalid(make_set_add_delta(10, -6));
    val_expect_int64(5);

    // conflicting subs bad
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_set_add_delta(10, -5));
    check_invalid(make_set_add_delta(10, -6));
    val_expect_int64(5);

    // invalid single delta
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_invalid(make_set_add_delta(10, -11));
    val_expect_nullopt();

    // check overflow multi subs
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_set_add_delta(INT64_MAX, INT64_MIN + 10));
    check_invalid(make_set_add_delta(INT64_MAX, INT64_MIN + 10));
    val_expect_int64(9);
        
    // check pos overflow
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_set_add_delta(INT64_MAX, 1));
    check_valid(make_set_add_delta(INT64_MAX, INT64_MAX));
    check_valid(make_set_add_delta(INT64_MAX, INT64_MAX));
    val_expect_int64(INT64_MAX);

    check_valid(make_set_add_delta(INT64_MAX, INT64_MIN + 10));
    // delta is currently INT64_MAX + 9

    val_expect_int64(INT64_MAX);
    check_valid(make_set_add_delta(INT64_MAX, -10));
    val_expect_int64(INT64_MAX - 1);
        
    // check neg overflow
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_invalid(make_set_add_delta(INT64_MIN, -1));
    check_valid(make_set_add_delta(INT64_MIN, 1));
        

    // check neg overflow across multiple deltas
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_set_add_delta(INT64_MAX, INT64_MIN + 1));
    check_invalid(make_set_add_delta(INT64_MAX, INT64_MIN));
    val_expect_int64(0);

    // check neg overflow across multiple deltas 2
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_set_add_delta(INT64_MIN, 0));
    check_invalid(make_set_add_delta(INT64_MIN, -200));
    val_expect_int64(INT64_MIN);
}

TEST_F(ProxyApplicatorTests, HashsetOnlyTests)
{
    auto make_hash = [](uint64_t i) { return hash_xdr<uint64_t>(i); };

    // no base object section

    // insert one
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_hash_set_insert(make_hash(0), 0));
    val_expect_hash(make_hash(0));
        
    // insert several

    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_hash_set_insert(make_hash(0), 0));
    check_valid(make_hash_set_insert(make_hash(1), 0));
    check_valid(make_hash_set_insert(make_hash(2), 0));
    check_valid(make_hash_set_insert(make_hash(3), 0));
    check_valid(make_hash_set_insert(make_hash(4), 0));

    check_invalid(make_hash_set_insert(make_hash(0), 0));
    check_invalid(make_hash_set_insert(make_hash(4), 0));

    val_expect_hash(make_hash(0));


    // tiny limit section
    StorageObject obj;
    obj.body.type(ObjectType::HASH_SET);
    obj.body.hash_set().max_size = 1;

    // insert one
    applicator = std::make_unique<ProxyApplicator>(obj);
    check_valid(make_hash_set_insert(make_hash(0), 0));
    val_expect_hash(make_hash(0));

    // insert two
    applicator = std::make_unique<ProxyApplicator>(obj);
    check_valid(make_hash_set_insert(make_hash(0), 0));
    check_invalid(make_hash_set_insert(make_hash(1), 0));
    val_expect_hash(make_hash(0));
    val_nexpect_hash(make_hash(1));



    // raise limit
    applicator = std::make_unique<ProxyApplicator>(obj);
    check_valid(make_hash_set_increase_limit(64));
    check_valid(make_hash_set_increase_limit(64));

    auto res = applicator->get_deltas();

    ASSERT_EQ(res.size(), 1);
    ASSERT_EQ(res[0].type(), DeltaType::HASH_SET_INCREASE_LIMIT);
    ASSERT_EQ(res[0].limit_increase(), 128);

    check_valid(make_hash_set_insert(make_hash(0), 0));
    check_invalid(make_hash_set_insert(make_hash(1), 0));
    
    // clear immediate effect
    applicator = std::make_unique<ProxyApplicator>(obj);

    check_valid(make_hash_set_insert(make_hash(0), 0));
    check_valid(make_hash_set_clear(0));

    check_invalid(make_hash_set_insert(make_hash(0), 0));

    val_nexpect_hash(make_hash(0));

    res = applicator->get_deltas();

    auto find_clear = [&]() {
        for (auto const& d : res) {
            if (d.type() == DeltaType::HASH_SET_CLEAR) {
                return true;
            }
        }
        return false;
    };
    ASSERT_TRUE(find_clear());

    // can't insert after clear
    // nullopt, not obj, to ensure insert fails bc clear not bc limit
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);

    check_valid(make_hash_set_insert(make_hash(0), 0));
    check_valid(make_hash_set_clear(10));
    check_invalid(make_hash_set_insert(make_hash(5), 5));
    val_nexpect_hash(make_hash(5));


    // no hash limit inc means no delta output
    applicator = std::make_unique<ProxyApplicator>(obj);
    res = applicator->get_deltas();
    ASSERT_EQ(res.size(), 0);

    // increase limit by 0
    check_valid(make_hash_set_increase_limit(0));
    res = applicator->get_deltas();
    ASSERT_EQ(res.size(), 1);
    ASSERT_EQ(res[0].type(), DeltaType::HASH_SET_INCREASE_LIMIT);
    ASSERT_EQ(res[0].limit_increase(), 0);
}

TEST_F(ProxyApplicatorTests, AssetOnlyTests)
{
    // add to empty
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_asset_add(10));
    check_valid(make_asset_add(20));
    val_expect_amount(30);
    

    // sub to empty
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_invalid(make_asset_add(-10));
    ASSERT_TRUE(applicator -> get_deltas().size() == 0);
    

    // add but sub fails
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_asset_add(5));
    check_invalid(make_asset_add(-10));
    val_expect_amount(5);
    expect_delta(5);
    

    // overflow not on value but on delta
    
    applicator = std::make_unique<ProxyApplicator>(std::nullopt);
    check_valid(make_asset_add(INT64_MAX));

    check_valid(make_asset_add(-10));
    check_invalid(make_asset_add(11));

    val_expect_amount(INT64_MAX - 10);
    expect_delta(INT64_MAX - 10);
} 


} // namespace scs
