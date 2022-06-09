#include "state_db/serial_delta_batch.h"

namespace scs 
{

void 
SerialDeltaBatch::add_deltas(const AddressAndKey& key, DeltaVector&& dv)
{
	if (dv.size() == 0) {
		return;
	}
	
	auto it = deltas.find(key);
	if (it == deltas.end()) {
		//throw std::runtime_error("can't lookup without preloading obj into batch");
		it = deltas.emplace(key, value_t()).first;
	}
	it->second.vec.add(std::move(dv));
}

} /* scs */
