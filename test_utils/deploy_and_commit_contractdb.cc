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

#include "test_utils/deploy_and_commit_contractdb.h"

#include <catch2/catch_test_macros.hpp>

#include "crypto/hash.h"

#include "contract_db/contract_db.h"
#include "contract_db/contract_db_proxy.h"

#include "storage_proxy/transaction_rewind.h"

namespace scs {

namespace test {

void
deploy_and_commit_contractdb(ContractDB& contract_db,
                             const Address& addr,
                             std::shared_ptr<const Contract> contract)
{
    ContractDBProxy proxy(contract_db);

    Hash h = hash_xdr(*contract);

    proxy.create_contract(contract);

    REQUIRE(proxy.deploy_contract(addr, h));

    {
        TransactionRewind rewind;

        REQUIRE(proxy.push_updates_to_db(rewind));

        rewind.commit();
    }

    contract_db.commit();

    ContractDBProxy proxy_new(contract_db);

    REQUIRE(proxy_new.get_script(addr).data);
}

} // namespace test
} // namespace scs
