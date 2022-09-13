#pragma once

#include <optional>
#include <string>

#include "xdr/storage.h"

namespace debug {

std::string
storage_object_to_str(std::optional<scs::StorageObject> const& obj);

std::string
storage_object_to_str(scs::StorageObject const& obj);

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
