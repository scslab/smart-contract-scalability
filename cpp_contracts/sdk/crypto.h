#pragma once

#include "sdk/macros.h"
#include "sdk/concepts.h"
#include "sdk/alloc.h"
#include "sdk/types.h"
#include "sdk/concepts.h"

#include <cstdint>
#include <optional>

namespace sdk
{

namespace detail
{

BUILTIN("hash")
void
hash(
	uint32_t input_offset,
	uint32_t input_len,
	uint32_t output_offset);

BUILTIN("check_sig_ed25519")
uint32_t
check_sig_ed25519(
	uint32_t pk_offset,
	/* pk len = 32 */
	uint32_t sig_offset,
	/* sig_len = 64 */
	uint32_t msg_offset,
	uint32_t msg_len);

} /* detail */

Hash hash(uint8_t const* data, uint32_t size)
{
	Hash out;
	detail::hash(to_offset(data), size, to_offset(out.data()));
	return out;
}

template <VectorLike T>
Hash hash(T const& object)
{
	return hash(object.data(), object.size());
}

template <VectorLike T>
void hash(T const& object, StorageKey& out)
{
	detail::hash(to_offset(object.data()), object.size(), to_offset(out.data()));
}
/*
bool check_sig_ed25519(
	const PublicKey& pk,
	const Signature& sig,
	const Hash& msg_hash)
{
	return detail::check_sig_ed25519(
		to_offset(pk.data()),
		to_offset(sig.data()),
		to_offset(msg_hash.data()),
		sizeof(Hash)) != 0;
} */

template<TriviallyCopyable msg>
bool
check_sig_ed25519(
	const PublicKey& pk,
	const Signature& sig,
	const msg& m)
{
	return detail::check_sig_ed25519(
	to_offset(pk.data()),
	to_offset(sig.data()),
	to_offset(&m),
	sizeof(msg)) != 0;
}

} /* sdk */
