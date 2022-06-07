#include "state_db/state_db.h"
#include "state_db/delta_batch.h"
#include "state_db/delta_batch.h"

namespace scs
{

std::optional<StorageObject>
StateDB::get(AddressAndKey const& a) const
{
	auto it = state_db.find(a);
	if (it == state_db.end())
	{
		return std::nullopt;
	}
	return it -> second;
}

void 
StateDB::populate_delta_batch(DeltaBatch& delta_batch) const
{
	auto& map = delta_batch.deltas;
	for (auto& [k, v] : map)
	{
		auto it = state_db.find(k);
		if (it != state_db.end())
		{
			v.mutator.populate_base(it->second);
		}
	}

	delta_batch.populated = true;
}

void 
StateDB::apply_delta_batch(DeltaBatch const& delta_batch)
{
	if (!delta_batch.applied)
	{
		throw std::runtime_error("can't apply before filtering (can't filter without populating batch with base values)");
	}
	auto const& map = delta_batch.deltas;
	for (auto const& [k, v] : map)
	{
		auto res = v.mutator.get_object();
		if (res)
		{
			state_db[k] = *res;
		}
	}
}

} /* scs */
