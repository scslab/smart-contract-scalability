#include "state_db/state_db.h"
#include "state_db/delta_batch.h"
#include "state_db/serial_delta_batch.h"

namespace scs
{

void 
StateDB::populate_delta_batch(SerialDeltaBatch& delta_batch) const
{
	auto& map = delta_batch.get_delta_map();
	for (auto& [k, v] : map)
	{
		auto it = state_db.find(k);
		if (it != state_db.end())
		{
			v.second.populate_base(it->second);
		}
	}
}

void 
StateDB::apply_delta_batch(DeltaBatch const& delta_batch)
{
	auto const& map = delta_batch.get_delta_map();
	for (auto const& [k, v] : map)
	{
		auto res = v.second.get_object();
		if (res)
		{
			state_db[k] = *res;
		}
	}
}

} /* scs */
