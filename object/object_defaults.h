#pragma once

#include "xdr/storage_delta.h"
#include "xdr/storage.h"

#include <optional>

namespace scs
{

std::optional<ObjectType>
object_type_from_delta_type(DeltaType d_type);

std::optional<StorageObject>
make_object_from_delta(StorageDelta const& d);

} /* scs */
