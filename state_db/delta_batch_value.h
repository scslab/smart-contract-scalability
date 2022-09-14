#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <vector>

#include "object/delta_type_class.h"
#include "state_db/delta_vec.h"

#include "filter/apply_context.h"
#include "filter/filter_context.h"

#include "utils/atomic_singleton.h"

namespace scs {

struct DeltaBatchValueContext
{

    std::unique_ptr<FilterContext> filter;
    std::unique_ptr<ApplyContext> applier;

    DeltaBatchValueContext(DeltaTypeClass const& tc)
        : filter(make_filter_context(tc))
        , applier(make_apply_context(tc))
    {}
};

class DeltaBatchValue
{
    // null until filtering starts
    std::unique_ptr<AtomicSingleton<DeltaBatchValueContext>> context;

    DeltaTypeClass tc;

    void error_guard() const;

  public:
    std::vector<DeltaVector> vectors;

    DeltaBatchValue();

    void make_context();
    void add_tc(const DeltaTypeClass& other);
    const DeltaTypeClass& get_tc() const;

    DeltaBatchValueContext& get_context();
    const DeltaBatchValueContext& get_context() const;
};

struct DeltaBatchValueMetadata
{
    // TODO compress into one 4 byte value?
    //  It should be _ok_ now, with only adding 8 bytes
    int32_t num_deltas = 0;
    int32_t num_vectors = 0;

    DeltaBatchValueMetadata& operator+=(const DeltaBatchValueMetadata& other)
    {
        num_deltas += other.num_deltas;
        num_vectors += other.num_vectors;
        return *this;
    }

    friend DeltaBatchValueMetadata operator-(DeltaBatchValueMetadata lhs,
                                             DeltaBatchValueMetadata const& rhs)
    {
        lhs.num_deltas -= rhs.num_deltas;
        lhs.num_vectors -= rhs.num_vectors;
        return lhs;
    }

    DeltaBatchValueMetadata operator-() const
    {
        return DeltaBatchValueMetadata{ .num_deltas = -this->num_deltas,
                                        .num_vectors = -this->num_vectors };
    }

    constexpr static DeltaBatchValueMetadata zero()
    {
        return DeltaBatchValueMetadata{ .num_deltas = 0, .num_vectors = 0 };
    }

    static DeltaBatchValueMetadata from_value(DeltaBatchValue const& val)
    {
        DeltaBatchValueMetadata meta = DeltaBatchValueMetadata::zero();
        for (auto const& v : val.vectors) {
            meta.num_vectors++;
            meta.num_deltas += v.size();
        }
        return meta;
    }
};

} // namespace scs
