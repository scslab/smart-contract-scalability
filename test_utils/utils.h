#pragma once

#include "mtt/trie/debug_macros.h"
#include "mtt/utils/serialize_endian.h"

#include "xdr/types.h"

#include <sodium.h>

#include <cstdio>

namespace scs
{

namespace test
{

[[maybe_unused]]
Address address_from_uint64(uint64_t addr)
{
	Address out;
	utils::write_unsigned_big_endian(out, addr);
	return out;	
}

[[maybe_unused]]
uint32_t method_name_from_human_readable(std::string const& str)
{
	if (sodium_init() < 0) {
		throw std::runtime_error("sodium init failed");
	}

	Hash hash_buf;

	const uint8_t* data = reinterpret_cast<const uint8_t*>(str.c_str());

	if (hash_buf.size() != crypto_hash_sha256_BYTES)
	{
		throw std::runtime_error("invalid hash format");
	}

	if (crypto_hash_sha256(
		hash_buf.data(), 
		data, str.length()) != 0) 
	{
		throw std::runtime_error("error in crypto_SHA256");
	}

	return utils::read_unsigned_little_endian<uint32_t>(hash_buf.data());
}

} /* test */

} /* scs */
