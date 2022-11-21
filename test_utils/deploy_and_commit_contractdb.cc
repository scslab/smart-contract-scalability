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

    REQUIRE(contract_db.get_script(addr, nullptr).data);
}

} // namespace test
} // namespace scs