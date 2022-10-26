#pragma once

#include "xdr/types.h"

#include <xdrpp/types.h>

#include "xdr/storage_delta.h"

namespace scs {

StorageDelta
make_raw_memory_write(xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& data);

StorageDelta
make_delete_last();

StorageDelta
make_nonnegative_int64_set_add(int64_t set, int64_t add);

StorageDelta
make_hash_set_insert(Hash const& h);

StorageDelta
make_hash_set_increase_limit(uint16_t limit);

StorageDelta
make_hash_set_clear();

} // namespace scs
