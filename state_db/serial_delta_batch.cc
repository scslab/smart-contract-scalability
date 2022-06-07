#include "state_db/serial_delta_batch.h"

namespace scs 
{

void 
SerialDeltaBatch::add_delta(const AddressAndKey& key, StorageDelta&& delta, DeltaPriority&& priority)
{
	auto it = deltas.find(key);
	if (it == deltas.end()) {
		//throw std::runtime_error("can't lookup without preloading obj into batch");
		it = deltas.emplace(key, value_t()).first;
	}
	it->second.vec.add_delta(std::move(delta), std::move(priority));
}

} /* scs */
