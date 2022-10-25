#include "crypto/crypto_utils.h"

#include <sodium.h>
#include <cstdio>
#include <stdexcept>

namespace scs
{

uint8_t* HASH_KEY;

//static uint8_t HASH_KEY[crypto_shorthash_KEYBYTES];

void 
initialize_crypto()
{
	int res = sodium_init();
	if (res == -1)
	{
		throw std::runtime_error("failed to init sodium");
	}
	if (res == 0)
	{
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

    if (crypto_shorthash(reinterpret_cast<uint8_t*>(&hash_out),
                         data,
                         data_len,
                         HASH_KEY)
        != 0) {
        throw std::runtime_error("shorthash fail");
    }

    // https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction
    return ((hash_out & 0xFFFF'FFFF) /* 32 bits */
            * static_cast<uint64_t>(modulus))
           >> 32;
}

} /* scs */
