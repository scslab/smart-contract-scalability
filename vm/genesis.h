#pragma once

#include "xdr/types.h"
#include <cstdint>

namespace scs
{

class ContractDB;

Address make_address(uint64_t a, uint64_t b, uint64_t c, uint64_t d);

const static Address
DEPLOYER_ADDRESS = make_address(0, 0, 0, 0);

void
install_genesis_contracts(ContractDB& contract_db);

}
