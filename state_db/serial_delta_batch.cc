#include "state_db/serial_delta_batch.h"

namespace scs 
{

void 
SerialDeltaBatch::add_delta(const AddressAndKey& key, StorageDelta&& delta, DeltaPriority&& priority)
{
	AddressAndKey ak;

	//static_assert(sizeof(AddressAndKey) == sizeof(Address) + sizeof(InvariantKey));

	//memcpy(ak.data(), addr.data(), addr.size());
	//memcpy(ak.data() + addr.size(), key.data(), key.size());

	deltas[ak].first.add_delta(std::move(delta), std::move(priority));
}

} /* scs */
