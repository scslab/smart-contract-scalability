#pragma once

#include "xdr/types.h"

namespace scs
{

void init_pedersen();


std::array<uint8_t, 32>
pedersen_commitment(unsigned __int128 value, Hash const& blinding);

}
