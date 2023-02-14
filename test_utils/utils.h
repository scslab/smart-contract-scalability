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

#include "mtt/trie/debug_macros.h"
#include "mtt/utils/serialize_endian.h"

#include "xdr/types.h"

#include <sodium.h>

#include <cstdio>

namespace scs {

namespace test {

[[maybe_unused]] Address
address_from_uint64(uint64_t addr)
{
    Address out;
    utils::write_unsigned_big_endian(out, addr);
    return out;
}

[[maybe_unused]] uint32_t
method_name_from_human_readable(std::string const& str)
{
    if (sodium_init() < 0) {
        throw std::runtime_error("sodium init failed");
    }

    Hash hash_buf;

    const uint8_t* data = reinterpret_cast<const uint8_t*>(str.c_str());

    if (hash_buf.size() != crypto_hash_sha256_BYTES) {
        throw std::runtime_error("invalid hash format");
    }

    if (crypto_hash_sha256(hash_buf.data(), data, str.length()) != 0) {
        throw std::runtime_error("error in crypto_SHA256");
    }

    return utils::read_unsigned_little_endian<uint32_t>(hash_buf.data());
}

} // namespace test

} // namespace scs
