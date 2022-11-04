#pragma once

#include <array>
#include <cstdint>

namespace sdk
{

typedef std::array<uint8_t, 32> Address;
typedef std::array<uint8_t, 32> StorageKey;
typedef std::array<uint8_t, 32> Hash;
typedef std::array<uint8_t, 32> PublicKey;
typedef std::array<uint8_t, 64> Signature;

struct EmptyStruct {};

} /* sdk */
