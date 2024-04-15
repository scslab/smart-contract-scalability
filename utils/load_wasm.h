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

#include <cstdio>

#include "xdr/types.h"
#include "contract_db/verified_script.h"

namespace scs {

[[maybe_unused]] std::shared_ptr<VerifiedScript> static load_wasm_from_file(
    const char* filename)
{
    FILE* f = std::fopen(filename, "r");

    if (f == nullptr) {
        throw std::runtime_error("failed to load wasm file");
    }

    std::vector<uint8_t> contents;

    const int BUF_SIZE = 65536;
    char buf[BUF_SIZE];

    int count = -1;
    while (count != 0) {
        count = std::fread(buf, sizeof(char), BUF_SIZE, f);
        if (count > 0) {
            contents.insert(contents.end(), buf, buf + count);
        }
    }
    std::fclose(f);

    return std::make_shared<VerifiedScript>(contents);
}

} // namespace scs
