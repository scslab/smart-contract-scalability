#include "state_db/serial_delta_batch.h"

#include "state_db/delta_batch.h"

namespace scs 
{

SerialDeltaBatch::SerialDeltaBatch(map_t& serial_trie)
	: deltas(serial_trie)
{}

struct AppendInsertFn
{
	using base_value_type = DeltaBatchValue;
	using prefix_t = trie::ByteArrayPrefix<sizeof(AddressAndKey)>;

	static
	base_value_type
	new_value(const prefix_t& prefix)
	{
		base_value_type out;
		out.vectors.push_back(std::make_unique<DeltaVector>());
		return out;
	}

	static void 
	value_insert(base_value_type& main_value, DeltaVector&& vec) {
		main_value.vectors.back()->add(std::move(vec));
	}
};


void 
SerialDeltaBatch::add_deltas(const AddressAndKey& key, DeltaVector&& dv)
{
	if (dv.size() == 0) {
		return;
	}

	deltas.template insert<AppendInsertFn>((key), std::move(dv));


	
/*	auto it = deltas.find(key);
	if (it == deltas.end()) {
		//throw std::runtime_error("can't lookup without preloading obj into batch");
		it = deltas.emplace(key, value_t()).first;
		it->second.vectors.push_back(std::make_unique<DeltaVector>());
	}
	it->second.vectors.back()->add(std::move(dv)); */
}

} /* scs */
