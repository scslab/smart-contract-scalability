#include "object/make_delta.h"

namespace scs {

StorageDelta
make_raw_memory_write(xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& data)
{
    StorageDelta d;
    d.type(DeltaType::RAW_MEMORY_WRITE);
    d.data() = std::move(data);
    return d;
}

StorageDelta
make_delete_last()
{
    StorageDelta d;
    d.type(DeltaType::DELETE_LAST);
    return d;
}

StorageDelta
make_nonnegative_int64_set_add(int64_t set, int64_t add)
{
    StorageDelta d;
    d.type(DeltaType::NONNEGATIVE_INT64_SET_ADD);
    d.set_add_nonnegative_int64().set_value = set;
    d.set_add_nonnegative_int64().delta = add;
    return d;
}

StorageDelta
make_hash_set_insert(Hash const& h, uint64_t threshold)
{
    StorageDelta d;
    d.type(DeltaType::HASH_SET_INSERT);
    d.hash() = HashSetEntry(h, threshold);
    return d;
}

StorageDelta
make_hash_set_insert(HashSetEntry const& entry)
{
    StorageDelta d;
    d.type(DeltaType::HASH_SET_INSERT);
    d.hash() = entry;
    return d;
}

StorageDelta
make_hash_set_increase_limit(uint16_t limit)
{
    StorageDelta d;
    d.type(DeltaType::HASH_SET_INCREASE_LIMIT);
    d.limit_increase() = limit;
    return d;
}

StorageDelta
make_hash_set_clear(uint64_t threshold)
{
    StorageDelta d;
    d.type(DeltaType::HASH_SET_CLEAR);
    d.threshold() = threshold;
    return d;
}

} // namespace scs
