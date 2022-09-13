#include "state_db/delta_batch.h"
#include "state_db/delta_vec.h"

#include "state_db/state_db.h"

#include "tx_block/tx_block.h"

namespace scs {

SerialDeltaBatch
DeltaBatch::get_serial_subsidiary()
{
    return SerialDeltaBatch(serial_cache.get(deltas));
}

struct DeltaBatchValueMergeFn
{
    static void value_merge(DeltaBatchValue& original_value,
                            DeltaBatchValue& merge_in_value)
    {

        /*
                        if (!original_value.context)
                        {
                                if (original_value.vectors.size() != 1)
                                {
                                        throw std::runtime_error("invalid
           original_value");
                                }
                                original_value.context =
           std::make_unique<DeltaBatchValueContext>(original_value.vectors.back()->get_typeclass_vote());
                        }

                        if (!merge_in_value.context)
                        {
                                if (merge_in_value.vectors.size() != 1)
                                {
                                        throw std::runtime_error("invalid
           merge_in_value");
                                }
                                auto tc_check =
           merge_in_value.vectors.back()->get_typeclass_vote(); if
           (original_value.context -> typeclass.is_lower_rank_than(tc_check))
                                {
                                        original_value.context -> typeclass =
           tc_check;
                                }
                        } else
                        {
                                auto const& tc_check = merge_in_value.context ->
           typeclass; if (original_value.context ->
           typeclass.is_lower_rank_than(tc_check))
                                {
                                        original_value.context -> typeclass =
           tc_check;
                                }
                        }
                        */

        std::move(merge_in_value.vectors.begin(),
                  merge_in_value.vectors.end(),
                  std::back_inserter(original_value.vectors));
        //	original_value.vectors.insert(original_value.vectors.end(),
        //		std::make_move_iterator(merge_in_value.vectors.begin()),
        //		std::make_move_iterator(merge_in_value.vectors.end()));

        original_value.add_tc(merge_in_value.get_tc());
    }
};

struct BatchApplyRange
{
    constexpr static int32_t GRAIN_SIZE = 1000;

    using TrieT = DeltaBatch::map_t;

    using ptr_t = TrieT::ptr_t;
    using allocator_t = TrieT::node_t::allocator_t;

    std::vector<ptr_t> work_list;

    using metadata_t = TrieT::metadata_t;

    metadata_t work_meta;

    // is not nullopt iff num_nodes == 1
    std::optional<std::pair<size_t, size_t>> vector_range;

    const allocator_t& allocator;

    bool empty() const { return work_meta.metadata.num_vectors == 0; }

    bool is_divisible() const
    {
        return work_meta.metadata.num_vectors > 1
               && work_meta.metadata.num_deltas > GRAIN_SIZE;
    }

    metadata_t get_metadata(ptr_t object)
    {
        return allocator.get_object(object).get_metadata();
    }

    BatchApplyRange(ptr_t work_root, const allocator_t& allocator)
        : work_list()
        , work_meta(metadata_t::zero())
        , allocator(allocator)
    {

        work_meta = get_metadata(work_root);

        work_list.push_back(work_root);
    }

    BatchApplyRange(BatchApplyRange& other, tbb::split)
        : work_list()
        , work_meta(metadata_t::zero())
        , allocator(other.allocator)
    {
        std::printf("start split\n");

        auto original_deltas = other.work_meta.metadata.num_deltas;
        if (original_deltas == 0) {
            return;
        }

        if (other.work_meta.size_ > 1) {
            while (work_meta.metadata.num_deltas < original_deltas / 2
                   && other.work_meta.size_ > 1) {
                if (other.work_list.size() == 0) {
                    // errors here do not get logged b/c tbb
                    throw std::runtime_error(
                        "other work list shouldn't be zero!");
                }

                if (other.work_list.size() == 1) {

                    if (other.work_list.at(0) == UINT32_MAX) {
                        throw std::runtime_error(
                            "found nullptr in ApplyRange!");
                    }
                    other.work_list
                        = allocator.get_object(other.work_list.at(0))
                              .children_list_nolock();
                } else {
                    work_list.push_back(other.work_list.at(0));
                    other.work_list.erase(other.work_list.begin());

                    auto m = get_metadata(
                        work_list
                            .back()); // allocator.get_object(work_list.back()).size();
                    work_meta += m;
                    other.work_meta += (-m);
                }
            }
        } else {
            work_meta.size_ = 1;

            if (other.work_list.size() != 1) {
                throw std::runtime_error(
                    "logic error somehow, should not have num_nodes 1 with "
                    "multiple work items");
            }

            work_list = other.work_list;

            if (!other.vector_range) {
                other.vector_range = std::make_optional(
                    std::make_pair(0, other.work_meta.metadata.num_vectors));
            }
            vector_range = std::make_optional(std::make_pair(
                other.vector_range->first, other.vector_range->first));

            auto const& val
                = allocator.get_object(work_list[0]).get_value(allocator);

            while ((work_meta.metadata.num_deltas < original_deltas / 2)
                   && (other.work_meta.metadata.num_vectors > 1)) {
                size_t steal_idx = other.vector_range->first;
                size_t sz = val.vectors[steal_idx].size();

                work_meta.metadata.num_deltas += sz;
                other.work_meta.metadata.num_deltas -= sz;

                work_meta.metadata.num_vectors++;
                other.work_meta.metadata.num_vectors--;

                other.vector_range->first++;
                vector_range->second++;
            }
        }
    }

    struct iterator
    {
        std::vector<ptr_t> work_list;
        // is not nullopt iff num_nodes == 1
        std::optional<std::pair<size_t, size_t>> vector_range;

        uint32_t work_list_idx;

        iterator(std::vector<ptr_t> work_list,
                 std::optional<std::pair<size_t, size_t>> vector_range,
                 uint32_t work_list_idx = 0)
            : work_list(work_list)
            , vector_range(vector_range)
            , work_list_idx(work_list_idx)
        {}

        ptr_t get_node_id() { return work_list[work_list_idx]; }

        std::optional<std::pair<size_t, size_t>> const& get_config()
        {
            return vector_range;
        }

        iterator& operator++(int)
        {
            work_list_idx++;
            if (work_list_idx >= work_list.size()) {
                work_list_idx = UINT32_MAX;
            }
            return *this;
        }

        bool operator==(const iterator& other)
        {
            return work_list_idx == other.work_list_idx;
        }
    };

    iterator begin() const { return iterator(work_list, vector_range); }

    iterator end() const
    {
        return iterator(
            std::vector<ptr_t>{ UINT32_MAX }, std::nullopt, UINT32_MAX);
    }
};

void
DeltaBatch::merge_in_serial_batches()
{
    if (populated) {
        throw std::runtime_error("add batches after populating");
    }

    deltas.template batch_merge_in<DeltaBatchValueMergeFn>(serial_cache);

    /*	auto& m = b->get_delta_map();

            for (auto& [k, v] : m)
            {
                    auto it = deltas.find(k);

                    auto tc = (*v.vectors.begin())->get_typeclass_vote();
                    if (it == deltas.end())
                    {
                            it = deltas.emplace(k, value_t(tc)).first;
                    }
                    //it->second.vec.add(std::move(v.vec));
                    if (it -> second.context->typeclass.is_lower_rank_than(tc))
                    {
                            it->second.context->typeclass = tc;
                    }
                    auto& main_vec = it->second.vectors;

                    main_vec.insert(main_vec.end(),
    std::make_move_iterator(v.vectors.begin()),
    std::make_move_iterator(v.vectors.end())); v.vectors.clear();
            }
    } */
}

struct FilterFn
{
    template<typename Applyable>
    void operator()(Applyable& work_root,
                    std::optional<std::pair<size_t, size_t>> config)
    {

        auto lambda = [config](DeltaBatchValue& dbv) {
            size_t vector_start = 0, vector_end = dbv.vectors.size();
            if (config) {
                vector_start = config->first;
                vector_end = config->second;
            }

            dbv.make_context();

            auto& context = dbv.get_context();

            for (size_t i = vector_start; i < vector_end; i++) {
                context.filter->add_vec(dbv.vectors[i]);
            }
        };

        work_root.apply(lambda);
    }
};

struct PruneFn
{
    TxBlock& txs;

    template<typename Applyable>
    void operator()(Applyable& work_root,
                    std::optional<std::pair<size_t, size_t>> config)
    {

        auto lambda = [this, config](DeltaBatchValue& dbv) {
            size_t vector_start = 0, vector_end = dbv.vectors.size();
            if (config) {
                vector_start = config->first;
                vector_end = config->second;
            }
            auto& context = dbv.get_context();

            for (size_t i = vector_start; i < vector_end; i++) {
                context.filter->prune_invalid_txs(dbv.vectors[i], txs);
            }
        };

        work_root.apply(lambda);
    }
};

struct ApplyFn
{
    TxBlock const& txs;

    template<typename Applyable>
    void operator()(Applyable& work_root,
                    std::optional<std::pair<size_t, size_t>> config)
    {

        auto lambda = [this, config](DeltaBatchValue& dbv) {
            size_t vector_start = 0, vector_end = dbv.vectors.size();
            if (config) {
                vector_start = config->first;
                vector_end = config->second;
            }

            auto& context = dbv.get_context();

            for (size_t i = vector_start; i < vector_end; i++) {
                context.applier->add_vec(dbv.vectors[i], txs);
            }
        };

        work_root.apply(lambda);
    }
};

void
DeltaBatch::filter_invalid_deltas(TxBlock& txs)
{
    if (!populated) {
        throw std::runtime_error(
            "cannot filter before populating with db values");
    }
    /*for (auto& [_, v] : deltas)
    {
            for (auto& dvs : v.vectors)
            {
                    v.context->dv_all.add(std::move(*dvs));
            }
            // TODO switch away from singlethreaded mutator
            v.context ->
    mutator.filter_invalid_deltas<TransactionFailurePoint::COMPUTE>(v.context ->
    dv_all, txs);
    } */
    FilterFn filter;
    PruneFn prune(txs);

    deltas.template parallel_batch_value_modify_customrange<FilterFn,
                                                            BatchApplyRange>(
        filter);
    deltas.template parallel_batch_value_modify_customrange<PruneFn,
                                                            BatchApplyRange>(
        prune);

    filtered = true;
}

void
DeltaBatch::apply_valid_deltas(TxBlock const& txs)
{
    if (!filtered) {
        throw std::runtime_error("cannot apply before filtering");
    }
    ApplyFn apply(txs);
    deltas.template parallel_batch_value_modify_customrange<ApplyFn,
                                                            BatchApplyRange>(
        apply);

    applied = true;
}

} // namespace scs
