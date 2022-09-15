#pragma once

#include "xdr/types.h"

#include <memory>

namespace scs {

class ContractDB;

namespace test {

void
deploy_and_commit_contractdb(ContractDB& contract_db,
                             const Address& addr,
                             std::unique_ptr<const Contract>&& contract);

}
} // namespace scs