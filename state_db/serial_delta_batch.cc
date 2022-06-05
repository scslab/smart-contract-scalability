#include "state_db/serial_delta_batch.h"

namespace scs 
{

void 
SerialDeltaBatch::add_delta(const Address& addr, const InvariantKey& key, StorageDelta&& delta, DeltaPriority&& priority)
{
	AddressAndKey ak;

	static_assert(sizeof(AddressAndKey) == sizeof(Address) + sizeof(InvariantKey));

	memcpy(ak.data(), addr.data(), addr.size());
	memcpy(ak.data() + addr.size(), key.data(), key.size());

	deltas[ak].first.add_delta(std::move(delta), std::move(priority));
}

void 
SerialDeltaBatch::add_delta_raw_memory_write(const Address& addr, const InvariantKey& key, xdr::opaque_vec<RAW_MEMORY_MAX_LEN>&& data, DeltaPriority&& priority)
{
	StorageDelta d;
	d.type(DeltaType::RAW_MEMORY_WRITE);
	d.data() = std::move(data);
	add_delta(addr, key, std::move(d), std::move(priority));
}


} /* scs */
