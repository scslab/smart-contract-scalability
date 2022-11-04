#pragma once

#include "sdk/types.h"

namespace sdk
{

namespace detail
{

BUILTIN("create_contract")
void create_contract(uint32_t contract_idx, uint32_t hash_out);

BUILTIN("deploy_contract")
void deploy_contract(uint32_t hash_offset, uint64_t nonce, uint32_t address_offset_out);

} // namespace detail

Hash create_contract(uint32_t contract_idx)
{
	Hash out;
	detail::create_contract(contract_idx, to_offset(&out));
	return out;
}

Address
deploy_contract(Hash const& h, uint64_t nonce)
{
	Address out;
	detail::deploy_contract(to_offset(&h), nonce, to_offset(&out));
	return out;
}

} // namespace sdk
