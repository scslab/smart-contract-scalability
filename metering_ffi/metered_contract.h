#pragma once

#include <cstdint>
#include <memory>

#include <utils/non_movable.h>

#include "xdr/types.h"

namespace scs {

namespace detail {

// meters metering/inject_metering/src/lib.rs:metered_contract
struct metered_contract
{
    uint8_t* data;
    uint32_t len;
    uint32_t capacity;
};

} // namespace detail

class MeteredContract : public utils::NonMovableOrCopyable
{

    detail::metered_contract base;

  public:
    MeteredContract(std::shared_ptr<const Contract> unmetered);

    ~MeteredContract();

    uint8_t const* data() const { return base.data; }

    uint32_t size() const { return base.len; }
};

} // namespace scs
