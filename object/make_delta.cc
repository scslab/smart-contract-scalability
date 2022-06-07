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

StorageDelta make_delete()
{
	StorageDelta d;
	d.type(DeltaType::DELETE);
	return d;
}

} /* scs */
