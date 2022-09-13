#include "state_db/state_db.h"
#include "state_db/delta_batch.h"

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

#include "state_db/modified_keys_list.h"

namespace scs {

std::optional<StorageObject> 
StateDB::get_committed_value(const AddressAndKey& a) const
{
	auto const* res = state_db.get_value_nolocks(a);
	if (res)
	{
		return (*res).v -> get_committed_object();
	}
	return std::nullopt;
}

std::optional<RevertableObject::DeltaRewind>
StateDB::try_apply_delta(const AddressAndKey& a, const StorageDelta& delta)
{
	auto* res = state_db.get_value_nolocks(a);
	if (!res)
	{
		return new_key_cache.try_reserve_delta(a, delta);
	} else
	{
		return (*res).v -> try_add_delta(delta);
	}
}


struct UpdateFn
{
	NewKeyCache& new_key_cache;
    StateDB::trie_t& main_db;
    StateDB::cache_t& new_kvs;

    using prefix_t = StateDB::prefix_t;

    template<typename Applyable>
    void operator()(const Applyable& work_root)
    {
        // Must guarantee no concurrent modification of commitment_trie (other
        // than values)

        auto* main_db_subnode = main_db.get_subnode_ref_nolocks(
            work_root.get_prefix(), work_root.get_prefix_len());


        auto apply_lambda = [this, main_db_subnode]
        (const prefix_t& addrkey, const trie::EmptyValue&)
        {
        	auto* main_db_value = main_db_subnode->get_value_nolocks(addrkey);
        	if (main_db_value)
        	{
        		main_db_subnode->invalidate_hash_to_key_nolocks(addrkey);

        		main_db_value->v->commit_round();

        		if (!(main_db_value->v -> get_committed_object()))
        		{
        			if (!main_db.mark_for_deletion_nolocks(addrkey))
        			{
        				std::printf("failed to delete something\n");
        				std::fflush(stdout);
        				throw std::runtime_error("deletion failed");
        			}
        		}
        	} else
        	{

        		AddressAndKey query = addrkey.template get_bytes_array<AddressAndKey>();

        		std::optional<StorageObject> new_obj = new_key_cache.commit_and_get(query);
            	
            	if (new_obj)
            	{
            		new_kvs.get().insert(addrkey, std::move(*new_obj));
            	}
        	}

        };

        work_root.apply_to_kvs(apply_lambda);
        main_db.invalidate_hash_to_node_nolocks(main_db_subnode);
    }
};

void 
StateDB::commit_modifications(const ModifiedKeysList& list)
{
    cache_t new_kvs;

    new_key_cache.finalize_modifications();

    UpdateFn update(new_key_cache, state_db, new_kvs);

    list.get_keys().parallel_batch_value_modify(update);

    state_db.template batch_merge_in<trie::NoDuplicateKeysMergeFn>(new_kvs);

    state_db.perform_marked_deletions();

    new_key_cache.clear_for_next_block();
}


#if 0
std::optional<StorageObject>
StateDB::get(AddressAndKey const& a) const
{
    CONTRACT_INFO("get on key %s", debug::array_to_str(a).c_str());
    auto res = state_db.get_value(a);
    if (res) {
        return *res;
    }
    return std::nullopt;
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

struct UpdateFn
{
    StateDB::trie_t& main_db;
    StateDB::cache_t& new_kvs;

    using prefix_t = StateDB::prefix_t;

    template<typename Applyable>
    void operator()(const Applyable& work_root)
    {
        // Must guarantee no concurrent modification of commitment_trie (other
        // than values)

        auto* main_db_subnode = main_db.get_subnode_ref_nolocks(
            work_root.get_prefix(), work_root.get_prefix_len());

        auto apply_lambda
            = [this, main_db_subnode](const prefix_t& addrkey,
                                      const DeltaBatchValue& dbv) {
                  if (dbv.get_context().applier->should_ignore()) {
                      return;
                  }

                  std::optional<StorageObject> new_obj
                      = dbv.get_context().applier->get_object();

                  if (new_obj) {
                      auto update_fn = [&new_obj](StorageObject& old_obj) {
                          old_obj = *new_obj;
                      };
                      main_db_subnode->modify_value_nolocks(addrkey, update_fn);
                  } else {
                      main_db.mark_for_deletion(addrkey);
                  }
              };
        auto new_kv_lambda
            = [this](const prefix_t& addrkey, const DeltaBatchValue& dbv) {
                  if (dbv.get_context().applier->should_ignore()) {
                      return;
                  }
                  std::optional<StorageObject> obj
                      = dbv.get_context().applier->get_object();

                  if (obj) {
                      new_kvs.get().insert(addrkey, StateDB::value_t(*obj));
                  }
              };

        if (main_db_subnode == nullptr) {

            work_root.apply_to_kvs(new_kv_lambda);

            return;
        }

        work_root.apply_to_kvs(apply_lambda);
        main_db.invalidate_hash_to_node_nolocks(main_db_subnode);
    }
};

void
StateDB::apply_delta_batch(DeltaBatch const& delta_batch)
{
    if (!delta_batch.applied) {
        throw std::runtime_error("can't write before apply");
    }

    if (delta_batch.written_to_state_db) {
        throw std::runtime_error("double write on delta_batch");
    }
    delta_batch.written_to_state_db = true;

    cache_t new_kvs;

    UpdateFn update(state_db, new_kvs);

    delta_batch.deltas.parallel_batch_value_modify(update);

    state_db.batch_merge_in(new_kvs);

    state_db.perform_marked_deletions();
}
#endif

} // namespace scs
