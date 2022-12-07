#pragma once

#include "xdr/types.h"

#include <stdexcept>

#include <sodium.h>
#include <xdrpp/marshal.h>

namespace scs
{

template<typename T>
concept XdrType
= requires (const T object)
{
	xdr::xdr_to_opaque(object);
};

//! Hash an xdr (serializable) type.
//! Must call sodium_init() prior to usage.
template<XdrType xdr_type>
Hash hash_xdr(const xdr_type& value) {
	Hash hash_buf;
	auto serialized = xdr::xdr_to_opaque(value);

	if (crypto_generichash(
		hash_buf.data(), hash_buf.size(), 
		serialized.data(), serialized.size(), 
		NULL, 0) != 0) 
	{
		throw std::runtime_error("error in crypto_generichash");
	}
	return hash_buf;
}

//! probably not the right name but ah well TODO rename
[[maybe_unused]]
static Hash 
hash_vec(std::vector<uint8_t> const& bytes)
{
	Hash hash_buf;

	if (crypto_generichash(
		hash_buf.data(), hash_buf.size(), 
		bytes.data(), bytes.size(), 
		NULL, 0) != 0) 
	{
		throw std::runtime_error("error in crypto_generichash");
	}
	return hash_buf;
}

} /* scs */
