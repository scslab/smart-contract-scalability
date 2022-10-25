#pragma once

#include <cstdint>

namespace scs {

void __attribute__((constructor)) initialize_crypto();

// uses shared key randomly generated at startup
uint32_t
shorthash(const uint8_t* data, uint32_t data_len, const uint32_t modulus);

} // namespace scs
