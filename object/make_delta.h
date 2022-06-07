#pragma once

#include "xdr/types.h"

#include <xdrpp/types.h>

#include "xdr/storage_delta.h"

namespace scs
{

StorageDelta make_raw_memory_write(xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& data);

StorageDelta make_delete();

} /* scs */
