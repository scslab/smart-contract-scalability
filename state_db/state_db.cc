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
	/*auto it = state_db.find(a);
	if (it == state_db.end())
	{
		return std::nullopt;
	}
	return it -> second; */
	auto res = state_db.get_value(a);
	if (res)
	{
		return *res;
	}
	return std::nullopt;

	//return state_db.get_value(a);
}

void 
StateDB::populate_delta_batch(DeltaBatch& delta_batch) const
{
	// do nothing
	// we'll deal with side effects during apply

	/*auto& map = delta_batch.deltas;
	for (auto& [k, v] : map)
	{
		auto it = state_db.find(k);
		if (it != state_db.end())
		{
			v.context -> mutator.populate_base(it->second);
		}
	} */


	delta_batch.populated = true;
}

struct UpdateFn {
	StateDB::trie_t& main_db;

	using prefix_t = StateDB::prefix_t;

	template<typename Applyable>
	void operator() (const Applyable& work_root) {
		//Must guarantee no concurrent modification of commitment_trie (other than values)

		auto* main_db_subnode = main_db.get_subnode_ref_nolocks(work_root.get_prefix(), work_root . get_prefix_len());

		if (main_db_subnode == nullptr) {
			throw std::runtime_error("get_subnode_ref_nolocks should not return nullptr ever");
		}



		auto apply_lambda = [this, main_db_subnode] (const prefix_t& addrkey, const DeltaBatchValue& dbv) {

			auto update_fn = [&dbv] (std::optional<StorageObject>& obj) {

				obj = dbv.get_context().applier->get_object();
			};

			main_db_subnode -> modify_value_nolocks(addrkey, update_fn);
		};

		work_root . apply_to_kvs(apply_lambda);
		main_db.invalidate_hash_to_node_nolocks(main_db_subnode);
	}
};

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

	UpdateFn update(state_db);

	delta_batch.deltas.parallel_batch_value_modify(update);

	/*&auto const& map = delta_batch.deltas;
	for (auto const& [k, v] : map)
	{
		CONTRACT_INFO("applying delta to key %s", debug::array_to_str(k).c_str());
		auto res = v.context -> mutator.get_object();
		if (res)
		{
			CONTRACT_INFO("writing %s", debug::storage_object_to_str(*res).c_str());
			state_db[k] = *res;
		} else
		{
			state_db.erase(k);
		}
	} */

	//throw std::runtime_error("unimplemented");
}

} /* scs */
