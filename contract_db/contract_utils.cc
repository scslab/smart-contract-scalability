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

#include "contract_db/contract_utils.h"

#include "crypto/hash.h"

namespace scs {


// Guarantee -- no collisions on deploy contracts
Address
compute_contract_deploy_address(Address const& deployer_address,
                                Hash const& contract_hash,
                                uint64_t nonce)
{
    std::vector<uint8_t> bytes;
    bytes.insert(bytes.end(), deployer_address.begin(), deployer_address.end());
    bytes.insert(bytes.end(), contract_hash.begin(), contract_hash.end());
    bytes.insert(bytes.end(),
                 reinterpret_cast<uint8_t*>(&nonce),
                 reinterpret_cast<uint8_t*>(&nonce) + sizeof(uint64_t));

    return hash_vec(bytes);
}
 
void write_uint64_t(uint8_t* out, uint64_t value)
{
    for (size_t i = 0; i < 8; i++)
    {
        out[i] = (value >> (8*i)) & 0xFF;
    }
}

InvariantKey 
make_static_key(
    uint64_t a,
    uint64_t b,
    uint64_t c,
    uint64_t d)
{
    InvariantKey out;
    write_uint64_t(out.data(), a);
    write_uint64_t(out.data() + 8, b);
    write_uint64_t(out.data() + 16, c);
    write_uint64_t(out.data() + 24, d);
    return out;
}

} // namespace scs
