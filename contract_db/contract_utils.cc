#include "contract_db/contract_utils.h"

#include "crypto/hash.h"

namespace scs
{

Address
compute_contract_deploy_address(
	Address const& deployer_address,
	Hash const& contract_hash,
	uint64_t nonce)
{
	std::vector<uint8_t> bytes;
	bytes.insert(bytes.end(), deployer_address.begin(), deployer_address.end());
	bytes.insert(bytes.end(), contract_hash.begin(), contract_hash.end());
	bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&nonce), reinterpret_cast<uint8_t*>(&nonce) + sizeof(uint64_t));

	return hash_vec(bytes);
}


}