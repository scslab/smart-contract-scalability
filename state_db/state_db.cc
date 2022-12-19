#include "state_db/state_db.h"

#include "debug/debug_macros.h"
#include "debug/debug_utils.h"

#include "state_db/modified_keys_list.h"

#include <utils/time.h>

namespace scs {

void 
StateDB::assert_not_uncommitted_deltas() const
{
    if (has_uncommitted_deltas)
    {
        throw std::runtime_error("assert not uncommitted deltas fail");
    }
}

std::optional<StorageObject>
StateDB::get_committed_value(const AddressAndKey& a) const
{
    auto const* res = state_db.get_value(a);
    if (res) {
        //std::printf("got something for %s\n", debug::array_to_str(a).c_str());
        return (*res).v->get_committed_object();
    }
    //std::printf("got nothing for %s\n",debug::array_to_str(a).c_str());
    return std::nullopt;
}

std::optional<RevertableObject::DeltaRewind>
StateDB::try_apply_delta(const AddressAndKey& a, const StorageDelta& delta)
{
    auto* res = state_db.get_value(a);

    has_uncommitted_deltas.store(true, std::memory_order_relaxed);

    if (!res) {
        return new_key_cache.try_reserve_delta(a, delta);
    } else {
        return (*res).v->try_add_delta(delta);
    }
}

struct UpdateFn
{
    NewKeyCache& new_key_cache;
    StateDB::trie_t& main_db;
    using prefix_t = StateDB::prefix_t;

    template<typename Applyable>
    void operator() (const Applyable& work_root)
    {
     //   std::printf("thread %s start applyable %s\n", utils::ThreadlocalIdentifier::get_string().c_str(), work_root.get_prefix().to_string(work_root.get_prefix_len()).c_str());
     //   try {
        auto* main_db_subnode = main_db.get_subnode_ref_and_invalidate_hash(work_root.get_prefix(), work_root.get_prefix_len());

        if (main_db_subnode -> get_prefix_len () != work_root.get_prefix_len())
        {
            std::printf("len mismatch got %s wanted %s\n", 
                main_db_subnode -> get_prefix().to_string(main_db_subnode->get_prefix_len()).c_str(),
                work_root.get_prefix().to_string(work_root.get_prefix_len()).c_str());
            std::fflush(stdout);
            throw std::runtime_error("wtf");
        }

     //   main_db_subnode -> log(std::string("thread ") + utils::ThreadlocalIdentifier::get_string() +" init:");

        auto apply_lambda = [this, main_db_subnode, &work_root](const prefix_t& addrkey, const trie::EmptyValue&)
        {
           // std::printf("thread %s apply_lambda to key %s\n", utils::ThreadlocalIdentifier::get_string().c_str(), addrkey.to_string(trie::PrefixLenBits{512}).c_str());
            auto* main_db_value = main_db_subnode -> get_value(addrkey);
            if (main_db_value && (main_db_value -> v))
            {
                main_db_subnode -> invalidate_hash_to_key(addrkey);

                main_db_value -> v -> commit_round();

                if (!(main_db_value -> v -> get_committed_object()))
                {
                //    std::printf("thread %s deleting preexisting key %s\n", utils::ThreadlocalIdentifier::get_string().c_str(), addrkey.to_string(trie::PrefixLenBits{512}).c_str());
                    main_db_subnode -> delete_value(addrkey, main_db.get_gc());
                }
            }
            else
            {
                AddressAndKey query
                    = addrkey.template get_bytes_array<AddressAndKey>();

                std::optional<StorageObject> new_obj
                    = new_key_cache.commit_and_get(query);

                if (new_obj) {
                    main_db_subnode -> template insert<trie::OverwriteInsertFn<StateDB::value_t>>(addrkey, std::move(*new_obj), main_db.get_gc());
                   // new_kvs.get().insert(addrkey, std::move(*new_obj));
                } else
                {
                    if (main_db_subnode -> is_leaf())
                    {
                       // std::printf("thread %s deleting new value %s\n", utils::ThreadlocalIdentifier::get_string().c_str(), addrkey.to_string(trie::PrefixLenBits{512}).c_str());
                        main_db_subnode -> delete_value(addrkey, main_db.get_gc());
                    }
                }
            }
        };

            work_root.apply_to_kvs(apply_lambda);
       //     main_db_subnode -> log(std::string("thread ") + utils::ThreadlocalIdentifier::get_string() + " pre: ");
            main_db_subnode -> compute_hash_and_normalize(main_db.get_gc());
        //    main_db_subnode -> log(std::string("thread ") + utils::ThreadlocalIdentifier::get_string() + " post:");

    /*    }
        catch(std::exception& e)
        {
            std::printf(e.what());
            std::fflush(stdout);
            throw e;
        } */
    } 
};

/*
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

    //    std::printf("work root %s %d\n", work_root.get_prefix().to_string(work_root.get_prefix_len()).c_str(),
    //        work_root.get_prefix_len().len);

        auto* main_db_subnode = main_db.get_subnode_ref_nolocks(
            work_root.get_prefix(), work_root.get_prefix_len());

        auto apply_lambda = [this, main_db_subnode](const prefix_t& addrkey,
                                                    const trie::EmptyValue&) {
        //    std::printf("applying to key %s\n", addrkey.to_string(trie::PrefixLenBits(512)).c_str());
            auto* main_db_value = main_db_subnode->get_value_nolocks(addrkey);
            if (main_db_value) {
                main_db_subnode->invalidate_hash_to_key_nolocks(addrkey);

                main_db_value->v->commit_round();

                if (!(main_db_value->v->get_committed_object())) {
                    if (!main_db.mark_for_deletion_nolocks(addrkey)) {
                        std::printf("failed to delete something\n");
                        std::fflush(stdout);
                        throw std::runtime_error("deletion failed");
                    }
                }
            } else {

                AddressAndKey query
                    = addrkey.template get_bytes_array<AddressAndKey>();

                std::optional<StorageObject> new_obj
                    = new_key_cache.commit_and_get(query);

                if (new_obj) {
                    new_kvs.get().insert(addrkey, std::move(*new_obj));
                }
            }
        };

        work_root.apply_to_kvs(apply_lambda);
        main_db.invalidate_hash_to_node_nolocks(main_db_subnode);

	if (main_db_subnode)
	{
//		std::printf("main subnode %p\n", main_db_subnode);
		std::vector<uint8_t> digest_bytes;
		std::optional<trie::HashLog<prefix_t>> log = std::nullopt;
        // need to lock main_db_subnode
		//main_db_subnode -> compute_hash(digest_bytes, log);
	}
    }
}; */

void
StateDB::commit_modifications(const ModifiedKeysList& list)
{
    auto ts = utils::init_time_measurement();
    //cache_t new_kvs;

    new_key_cache.finalize_modifications();

    UpdateFn update(new_key_cache, state_db);

    std::printf("parallel modify before %lf\n", utils::measure_time(ts));

    uint32_t grain_size = std::max<uint32_t>(1, list.size() / 100);
    // can't have update granularity be 1, or else we'll apply
    // on a value and get_subnode will throw
    list.get_keys().parallel_batch_value_modify<UpdateFn>(update, grain_size);

    std::printf("parallel modify after %lf\n", utils::measure_time(ts));

//    state_db.template batch_merge_in<TLCACHE_SIZE, trie::NoDuplicateKeysMergeFn>(new_kvs);

  //  state_db.perform_marked_deletions();

    new_key_cache.clear_for_next_block();

    state_db.hash_and_normalize();

    std::printf("clear and root hash %lf\n", utils::measure_time(ts));

    state_db.do_gc();
    has_uncommitted_deltas = false;
    std::printf("finish %lf\n", utils::measure_time(ts));
}

struct RewindFn
{
    StateDB::trie_t& main_db;

    using prefix_t = StateDB::prefix_t;

    template<typename Applyable>
    void operator()(const Applyable& work_root)
    {
        auto* main_db_subnode = main_db.get_subnode_ref_and_invalidate_hash(
                work_root.get_prefix(), work_root.get_prefix_len());

        auto apply_lambda = [this, main_db_subnode](const prefix_t& addrkey,
                                                    const trie::EmptyValue&) {
            auto* main_db_value = main_db_subnode->get_value(addrkey);
            if (main_db_value) {
                // no need to invalidate hashes when rewinding
                main_db_value -> v -> rewind_round();
            }
            // otherwise new key, don't need to do anything
        };

        work_root.apply_to_kvs(apply_lambda);

    }
};
/*
struct RewindFn
{
    StateDB::trie_t& main_db;


    using prefix_t = StateDB::prefix_t;

    template<typename Applyable>
    void operator()(const Applyable& work_root)
    {
         auto* main_db_subnode = main_db.get_subnode_ref_nolocks(
            work_root.get_prefix(), work_root.get_prefix_len());

        auto apply_lambda = [this, main_db_subnode](const prefix_t& addrkey,
                                                    const trie::EmptyValue&) {
            auto* main_db_value = main_db_subnode->get_value_nolocks(addrkey);
            if (main_db_value) {
                // no need to invalidate hashes when rewinding
                main_db_value -> v -> rewind_round();
            }
            // otherwise new key, don't need to do anything
        };

        work_root.apply_to_kvs(apply_lambda);
        // we're rewinding, so no need to invalidate any hashes -- we're un-invalidating them here.
    }
}; */

void 
StateDB::rewind_modifications(const ModifiedKeysList& list)
{
    new_key_cache.clear_for_next_block();

    //throw std::runtime_error("unimpl");
    RewindFn rewind(state_db);

    list.get_keys().parallel_batch_value_modify<RewindFn>(rewind, 1);

    state_db.hash_and_normalize();

    has_uncommitted_deltas = false;
}

Hash
StateDB::hash()
{
    assert_not_uncommitted_deltas();

    auto h = state_db.hash_and_normalize();
    Hash out;
    std::memcpy(
        out.data(),
        h.data(),
        h.size());
    return out;
//    state_db.hash(out);
//    return out;
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
