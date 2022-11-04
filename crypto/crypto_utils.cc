#include "crypto/crypto_utils.h"

#include <atomic>
#include <cstdio>
#include <sodium.h>
#include <stdexcept>

namespace scs {

std::atomic<bool> hash_key_init = false;
uint8_t* HASH_KEY = nullptr;

void
initialize_crypto()
{
    int res = sodium_init();
    if (res == -1) {
        throw std::runtime_error("failed to init sodium");
    }
    bool expect = false;
    if (hash_key_init.compare_exchange_strong(expect, true)) {
        HASH_KEY = new uint8_t[crypto_shorthash_KEYBYTES];
        crypto_shorthash_keygen(HASH_KEY);
    }

    std::printf("initialized sodium\n");
}

uint32_t
shorthash(const uint8_t* data, uint32_t data_len, const uint32_t modulus)
{
    static_assert(crypto_shorthash_BYTES == 8);

    uint64_t hash_out;

    if (crypto_shorthash(
            reinterpret_cast<uint8_t*>(&hash_out), data, data_len, HASH_KEY)
        != 0) {
        throw std::runtime_error("shorthash fail");
    }

    // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction
    return ((hash_out & 0xFFFF'FFFF) /* 32 bits */
            * static_cast<uint64_t>(modulus))
           >> 32;
}

bool
check_sig_ed25519(PublicKey const& pk, Signature const& sig, std::vector<uint8_t> const& msg)
{
    return crypto_sign_verify_detached(
        sig.data(), msg.data(), msg.size(), pk.data()) == 0;

}


} // namespace scs
