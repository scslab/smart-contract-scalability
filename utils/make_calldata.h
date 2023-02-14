#pragma once

/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <vector>
#include <xdrpp/types.h>
#include <type_traits>

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

    if constexpr (!std::is_empty<calldata>::value)
    {
        out.insert(out.end(), addr, addr + sizeof(calldata));
    }

    auto rest = make_calldata(additional...);

    out.insert(out.end(), rest.begin(), rest.end());
    return out;
}

} // namespace scs
