#include "object/make_delta.h"

namespace scs
{

StorageDelta make_raw_memory_write(xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& data)
{
	StorageDelta d;
	d.type(DeltaType::RAW_MEMORY_WRITE);
	d.data() = std::move(data);
	return d;
}

StorageDelta make_delete_last()
{
	StorageDelta d;
	d.type(DeltaType::DELETE_LAST);
	return d;
}

StorageDelta make_nonnegative_int64_set_add(int64_t set, int64_t add)
{
	StorageDelta d;
	d.type(DeltaType::NONNEGATIVE_INT64_SET_ADD);
	d.set_add_nonnegative_int64().set_value = set;
	d.set_add_nonnegative_int64().delta = add;
	return d;
}

} /* scs */
