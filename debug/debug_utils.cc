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

#include "debug/debug_utils.h"

#include "mtt/common/debug_macros.h"

namespace debug {

std::string
storage_object_to_str(std::optional<scs::StorageObject> const& obj)
{
    if (!obj) {
        return "[nullopt]";
    }
    return storage_object_to_str(*obj);
}

std::string
storage_object_to_str(scs::StorageObject const& obj)
{
    switch (obj.body.type()) {
        case scs::ObjectType::RAW_MEMORY:
            return "[raw_memory: "
                   + debug::array_to_str(obj.body.raw_memory_storage().data)
                   + "]";
        case scs::ObjectType::NONNEGATIVE_INT64:
            return "[nn_int64: " + std::to_string(obj.body.nonnegative_int64())
                   + "]";
        case scs::ObjectType::HASH_SET: {
            std::string out = "hash_set: max_size = ";
            out += std::to_string(obj.body.hash_set().max_size);
            for (auto const& h : obj.body.hash_set().hashes) {
                out += " <" + array_to_str(h.hash.data(), h.hash.size()) + " "
                       + std::to_string(h.index) + ">";
            }
            return out + "]";
        }
    }
    throw std::runtime_error("unknown object type in storage_object_to_str");
}

std::string
storage_delta_to_str(scs::StorageDelta const& delta)
{
    switch (delta.type()) {
        case scs::DeltaType::DELETE_LAST:
            return "[delete_last]";
        case scs::DeltaType::RAW_MEMORY_WRITE:
            return "[raw memory write: " + debug::array_to_str(delta.data())
                   + "]";
        case scs::DeltaType::NONNEGATIVE_INT64_SET_ADD:
            return "[nn_int64: set "
                   + std::to_string(delta.set_add_nonnegative_int64().set_value)
                   + " "
                   + std::to_string(delta.set_add_nonnegative_int64().delta)
                   + "]";
        case scs::DeltaType::HASH_SET_INSERT:
            return "[hs insert: " + debug::array_to_str(delta.hash().hash)
                   + " tag " + std::to_string(delta.hash().index) + "]";
        case scs::DeltaType::HASH_SET_INCREASE_LIMIT:
            return "[hs limit inc: " + std::to_string(delta.limit_increase())
                   + "]";
        default:
            return "[unimplemented]";
    }
}

std::string
array_to_str(const unsigned char* array, const int len)
{
    std::stringstream s;
    s.fill('0');
    for (int i = 0; i < len; i++) {
        s << std::setw(2) << std::hex << (unsigned short)array[i];
    }
    return s.str();
}

} // namespace debug
