#pragma once

#include "xdr/storage_delta.h"
#include "xdr/storage.h"

#include <optional>

namespace scs
{

std::optional<StorageObject>
make_default_object_by_delta(DeltaType d_type);

StorageObject
make_default_object_by_type(ObjectType type);

} /* scs */
