#include "metering_ffi/metered_contract.h"

#include <utils/time.h>

namespace scs {

namespace detail {

extern "C"
{

// does NOT take ownership of data
metered_contract add_metering_ext(uint8_t const* data, uint32_t len);

void free_metered_contract(metered_contract contract);

}

metered_contract timed_add_metering_ext(uint8_t const* data, uint32_t len)
{
    auto ts = utils::init_time_measurement();
    auto out = add_metering_ext(data, len);
    std::printf("metering duration: %lf\n", utils::measure_time(ts));
    return out;
}

} // namespace detail

MeteredContract::MeteredContract(std::shared_ptr<const Contract> unmetered)
    : base(detail::timed_add_metering_ext(unmetered->data(), unmetered->size()))
{}

MeteredContract::~MeteredContract()
{
    if (base.data != nullptr) {
        detail::free_metered_contract(base);
    }
}

} // namespace scs
