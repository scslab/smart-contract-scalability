#include "filter/filter_context.h"

#include "state_db/delta_vec.h"

#include "object/delta_type_class.h"
#include "tx_block/tx_block.h"

namespace scs {

FilterContext::FilterContext(DeltaTypeClass const& dtc)
    : dtc(dtc)
{}

bool
FilterContext::check_valid_dtc()
{
    return dtc.get_valence().tv.type() != TypeclassValence::TV_ERROR;
}

void
FilterContext::add_vec(DeltaVector const& v)
{
    if (!check_valid_dtc()) {
        return;
    }

    add_vec_when_tc_valid(v);
}

void
FilterContext::invalidate_entire_vector(DeltaVector const& v, TxBlock& txs)
{
    auto const& vs = v.get();
    for (auto const& [d, h] : vs) {
        txs.template invalidate<TransactionFailurePoint::CONFLICT_PHASE_1>(h);
    }
}

void
FilterContext::prune_invalid_txs(DeltaVector const& v, TxBlock& txs)
{
    if (!check_valid_dtc()) {
        invalidate_entire_vector(v, txs);
        return;
    }

    prune_invalid_txs_when_tc_valid(v, txs);
}

class NNInt64FilterContext : public FilterContext
{
    std::atomic<int64_t> subtracted_amount;
    std::atomic_flag local_failure;

  public:
    NNInt64FilterContext(DeltaTypeClass const& dtc)
        : FilterContext(dtc)
        , subtracted_amount(0)
        , local_failure()
    {}

    void add_vec_when_tc_valid(DeltaVector const& v) override final
    {
        int64_t local_subtracted_amount = 0;

        auto const& vs = v.get();
        for (auto const& [d, _] : vs) {
            // by typeclass, all d have same set_value and are of type
            // set_add_nn_int64

            int64_t delta = d.set_add_nonnegative_int64().delta;

            if (delta < 0) {
                if (__builtin_add_overflow_p(delta,
                                             local_subtracted_amount,
                                             static_cast<int64_t>(0))) {
                    local_failure.test_and_set();
                    break;
                }
                local_subtracted_amount += delta;
            }
        }

        while (true) {
            // can't just do fetch_sub because we need to check for overflow
            int64_t cur_value
                = subtracted_amount.load(std::memory_order_relaxed);

            if (__builtin_add_overflow_p(cur_value,
                                         local_subtracted_amount,
                                         static_cast<int64_t>(0))) {
                local_failure.test_and_set();
                return;
            }
            int64_t res = cur_value + local_subtracted_amount;
            if (subtracted_amount.compare_exchange_strong(
                    cur_value, res, std::memory_order_relaxed)) {
                return;
            }
        }
    }

    void prune_invalid_txs_when_tc_valid(DeltaVector const& v,
                                         TxBlock& txs) override final
    {
        bool failed = false;

        if (local_failure.test()) {
            failed = true;
        }

        int64_t base_val = dtc.get_valence().tv.set_value();
        int64_t sub_amt_load
            = subtracted_amount.load(std::memory_order_relaxed);

        if (base_val < 0 && sub_amt_load < 0) {
            failed = true;
        }

        else if (base_val + sub_amt_load < 0) {
            failed = true;
        }

        if (failed) {
            auto const& vs = v.get();
            for (auto const& [d, h] : vs) {
                int64_t delta = d.set_add_nonnegative_int64().delta;

                if (delta < 0) {
                    txs.template invalidate<
                        TransactionFailurePoint::CONFLICT_PHASE_1>(h);
                }
            }
        }
    }
};

class RawMemoryFilterContext : public FilterContext
{

  public:
    RawMemoryFilterContext(DeltaTypeClass const& dtc)
        : FilterContext(dtc)
    {}

    void add_vec_when_tc_valid(DeltaVector const&) override final {}

    void prune_invalid_txs_when_tc_valid(DeltaVector const&,
                                         TxBlock&) override final
    {}
};

class ErrorFilterContext : public FilterContext
{

  public:
    ErrorFilterContext(DeltaTypeClass const& dtc)
        : FilterContext(dtc)
    {}

    void add_vec_when_tc_valid(DeltaVector const& v) override final
    {
        throw std::runtime_error("should never be called");
    }

    void prune_invalid_txs_when_tc_valid(DeltaVector const& v,
                                         TxBlock& txs) override final
    {
        throw std::runtime_error("should never be called");
    }
};

class NoActionFilterContext : public FilterContext
{
  public:
    NoActionFilterContext(DeltaTypeClass const& dtc)
        : FilterContext(dtc)
    {}
    void add_vec_when_tc_valid(DeltaVector const&) override final {}
    void prune_invalid_txs_when_tc_valid(DeltaVector const& v,
                                         TxBlock& txs) override final
    {}
};

std::unique_ptr<FilterContext>
make_filter_context(DeltaTypeClass const& dtc)
{
    switch (dtc.get_valence().tv.type()) {
        case TypeclassValence::TV_FREE:
            // nothing to filter here
            return std::make_unique<NoActionFilterContext>(dtc);
        case TypeclassValence::TV_DELETE_FIRST:
            return std::make_unique<NoActionFilterContext>(dtc);
        case TypeclassValence::TV_RAW_MEMORY_WRITE:
            return std::make_unique<RawMemoryFilterContext>(dtc);
        case TypeclassValence::TV_NONNEGATIVE_INT64_SET:
            return std::make_unique<NNInt64FilterContext>(dtc);
        case TypeclassValence::TV_ERROR:
            return std::make_unique<ErrorFilterContext>(dtc);
        default:
            std::printf("unimplemented filter\n");
            std::fflush(stdout);
            throw std::runtime_error("unimpl filter");
    }
}

} // namespace scs
