#pragma once

#include <cstdint>

#include "xdr/types.h"

namespace scs
{

Address
compute_contract_deploy_address(
	Address const& deployer_address,
	Hash const& hash,
	uint64_t nonce);


}