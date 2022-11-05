#pragma once

#include <cstdint>
#include <vector>
#include <utility>

#include "xdr/types.h"

namespace scs {

void __attribute__((constructor)) initialize_crypto();

// uses shared key randomly generated at startup
uint32_t
shorthash(const uint8_t* data, uint32_t data_len, const uint32_t modulus);

bool
check_sig_ed25519(PublicKey const& pk, Signature const& sig, std::vector<uint8_t> const& msg);

Signature
sign_ed25519(SecretKey const& sk, uint8_t const* msg, uint32_t msg_len);

template<typename T>
Signature
sign_ed25519(SecretKey const& sk, T const& msg)
{
	return sign_ed25519(sk, msg.data(), msg.size());
}

std::pair<SecretKey, PublicKey> 
deterministic_key_gen(uint64_t seed);


} // namespace scs
