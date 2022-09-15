#pragma once

#include <vector>
#include <xdrpp/types.h>

namespace scs {
namespace test {

template<typename calldata>
xdr::opaque_vec<> static make_calldata(calldata const& data)
{
    const uint8_t* addr = reinterpret_cast<const uint8_t*>(&data);

    xdr::opaque_vec<> out;
    out.insert(out.end(), addr, addr + sizeof(calldata));
    return out;
}

} // namespace test
} // namespace scs
