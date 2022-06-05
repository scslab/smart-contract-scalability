#pragma once

#include "xdr/storage_delta.h"
#include "xdr/storage.h"

#include <optional>

namespace scs
{

std::optional<StorageObject>
make_default_object(DeltaType d_type);

} /* scs */
