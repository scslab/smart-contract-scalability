#pragma once

#include <string>
#include <optional>

#include "xdr/storage.h"

namespace debug
{

std::string 
storage_object_to_str(std::optional<scs::StorageObject> const& obj);

std::string 
storage_object_to_str(scs::StorageObject const& obj);

} /* debug */
