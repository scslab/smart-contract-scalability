#pragma once

#include "state_db/delta_vec.h"
#include "object/object_mutator.h"

#include "xdr/storage.h"
#include "xdr/types.h"

#include <map>
#include <vector>

namespace scs
{

class SerialDeltaBatch
{
	// accumulator for all deltas in a block.
	// ultimately will have threadlocal cache of these
	// Each vec is kept in sort order by priority

	struct value_t
	{
		DeltaVector vec;
		//std::optional<StorageObject> base_obj;

		/*value_t(std::optional<StorageObject> const& obj)
			: vec()
			, base_obj(obj)
			{} */
	};

	using map_t = std::map<AddressAndKey, value_t>;

	map_t deltas;

public:

	void add_deltas(const AddressAndKey& key, DeltaVector&& dv);

	map_t& get_delta_map() {
		return deltas;
	}

	const map_t& get_delta_map() const {
		return deltas;
	}
};

} /* scs */
