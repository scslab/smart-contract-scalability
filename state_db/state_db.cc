#include "state_db/state_db.h"
#include "state_db/delta_batch.h"
#include "state_db/delta_batch.h"

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

namespace scs
{

std::optional<StorageObject>
StateDB::get(AddressAndKey const& a) const
{
	CONTRACT_INFO("get on key %s", debug::array_to_str(a).c_str());
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
		throw std::runtime_error("can't write before apply");
	}

	if (delta_batch.written_to_state_db)
	{
		throw std::runtime_error("double write on delta_batch");
	}
	delta_batch.written_to_state_db = true;

	auto const& map = delta_batch.deltas;
	for (auto const& [k, v] : map)
	{
		CONTRACT_INFO("applying delta to key %s", debug::array_to_str(k).c_str());
		auto res = v.mutator.get_object();
		if (res)
		{
			CONTRACT_INFO("writing %s", debug::storage_object_to_str(*res).c_str());
			state_db[k] = *res;
		} else
		{
			state_db.erase(k);
		}
	}
}

} /* scs */
