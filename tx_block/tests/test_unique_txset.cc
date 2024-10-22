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

#include "tx_block/unique_tx_set.h"
#include "xdr/transaction.h"
#include "crypto/hash.h"

namespace scs {

class TxSetTests : public ::testing::Test {

 protected:
  

  UniqueTxSet set;

  auto add_tx(SignedTransaction const& stx) {
    auto h = hash_xdr(stx);
    return set.try_add_transaction(h, stx, NondeterministicResults());
  }

    auto simple_make_unique_tx(uint64_t nonce) {

        SignedTransaction t;
        t.tx.gas_limit = nonce;
        return t;
    };
};

TEST_F(TxSetTests, SingleInsertsGood)
{


    ASSERT_TRUE(add_tx(simple_make_unique_tx(0)));
    ASSERT_TRUE(add_tx(simple_make_unique_tx(1)));
    ASSERT_TRUE(add_tx(simple_make_unique_tx(2)));

    set.finalize();

    Block b;
    set.serialize_block(b);

    ASSERT_EQ(b.transactions.size(), 3);
}

TEST_F(TxSetTests, DoubleInsertsBad) 
{
    ASSERT_TRUE(add_tx(simple_make_unique_tx(0)));
    ASSERT_TRUE(add_tx(simple_make_unique_tx(1)));
    ASSERT_TRUE(!add_tx(simple_make_unique_tx(1)));

    set.finalize();

    Block b;
    set.serialize_block(b);

    ASSERT_EQ(b.transactions.size(), 2);
}

} // namespace scs
