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

#include <optional>
#include <string>

#include "xdr/storage.h"
#include "xdr/storage_delta.h"

namespace debug {

std::string
storage_object_to_str(std::optional<scs::StorageObject> const& obj);

std::string
storage_object_to_str(scs::StorageObject const& obj);

std::string
storage_delta_to_str(scs::StorageDelta const& delta);

//! Convert a byte array to a hex string.
std::string
array_to_str(const unsigned char* array, const int len);

template<typename ArrayLike>
[[maybe_unused]] 
static std::string
array_to_str(const ArrayLike& array)
{
    return array_to_str(array.data(), array.size());
}

} // namespace debug
