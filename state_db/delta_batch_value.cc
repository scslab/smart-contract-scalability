#include "state_db/delta_batch_value.h"

namespace scs {

DeltaBatchValue::DeltaBatchValue()
    : context()
    , tc()
    , vectors()
{
    vectors.emplace_back();
}

void
DeltaBatchValue::make_context()
{
    context = std::make_unique<AtomicSingleton<DeltaBatchValueContext>>();
}

void
DeltaBatchValue::add_tc(const DeltaTypeClass& other)
{
    if (context) {
        throw std::runtime_error("can't add to tc after initializing context!");
    }
    tc.add(other);
}

const DeltaTypeClass&
DeltaBatchValue::get_tc() const
{
    return tc;
}

void 
DeltaBatchValue::error_guard() const
{
    if (!context)
    {
        std::printf("invalid access on context\n");
        std::fflush(stdout);
        throw std::runtime_error("invalid access on context");
    }
}

DeltaBatchValueContext&
DeltaBatchValue::get_context()
{
    error_guard();
    return context->get(tc);
}

const DeltaBatchValueContext&
DeltaBatchValue::get_context() const
{
    error_guard();
    return context->get(tc);
}

} // namespace scs
