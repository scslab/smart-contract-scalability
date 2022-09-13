#pragma once

#include <array>
#include <cstdint>

namespace sdk
{

typedef std::array<uint8_t, 32> Address;
typedef std::array<uint8_t, 32> StorageKey;
typedef std::array<uint8_t, 32> Hash;

struct EmptyStruct {};

} /* sdk */
