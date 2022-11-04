#pragma once

#include <vector>
#include <xdrpp/types.h>

namespace scs {

xdr::opaque_vec<>
static
make_calldata()
{
    return {};
}

template<typename calldata, typename... calldatas>
xdr::opaque_vec<> static make_calldata(calldata const& data, calldatas const& ...additional)
{
    const uint8_t* addr = reinterpret_cast<const uint8_t*>(&data);

    xdr::opaque_vec<> out;
    out.insert(out.end(), addr, addr + sizeof(calldata));

    auto rest = make_calldata(additional...);

    out.insert(out.end(), rest.begin(), rest.end());
    return out;
}

} // namespace scs
